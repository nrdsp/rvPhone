#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <driver/spi_slave.h>

#include "eos.h"
#include "msgq.h"
#include "power.h"
#include "net.h"

#define SPI_GPIO_RTS        22
#define SPI_GPIO_CTS        21
#define SPI_GPIO_MOSI       23
#define SPI_GPIO_MISO       19
#define SPI_GPIO_SCLK       18
#define SPI_GPIO_CS         5

#define SPI_SIZE_BUF        (EOS_NET_SIZE_BUF + 4)
#define SPI_SIZE_HDR        3

static volatile char net_sleep = 0;

static EOSBufQ net_buf_q;
static unsigned char *net_bufq_array[EOS_NET_SIZE_BUFQ];

static EOSMsgQ net_send_q;
static EOSMsgItem net_sndq_array[EOS_NET_SIZE_SNDQ];

static SemaphoreHandle_t mutex;
static SemaphoreHandle_t semaph;
static TaskHandle_t net_xchg_task_handle;
static const char *TAG = "EOS NET";

static eos_net_fptr_t net_handler[EOS_NET_MAX_MTYPE];

static void bad_handler(unsigned char mtype, unsigned char *buffer, uint16_t len) {
    ESP_LOGE(TAG, "bad handler: %d len: %d", mtype, len);
}

// Called after a transaction is queued and ready for pickup by master. We use this to set the handshake line high.
static void _post_setup_cb(spi_slave_transaction_t *trans) {
    gpio_set_level(SPI_GPIO_CTS, 1);
}

// Called after transaction is sent/received. We use this to set the handshake line low.
static void _post_trans_cb(spi_slave_transaction_t *trans) {
    gpio_set_level(SPI_GPIO_CTS, 0);
}

static void net_xchg_task(void *pvParameters) {
    int wake = 0;
    int skip_msg = 0;
    unsigned char mtype = 0;
    unsigned char mtype_flags = 0;
    unsigned char *buffer;
    uint16_t len;
    unsigned char *buf_send = heap_caps_malloc(SPI_SIZE_BUF, MALLOC_CAP_DMA);
    unsigned char *buf_recv = heap_caps_malloc(SPI_SIZE_BUF, MALLOC_CAP_DMA);
    esp_err_t ret;

    static spi_slave_transaction_t spi_tr;

    //Configuration for the SPI bus
    static spi_bus_config_t spi_bus_cfg = {
        .mosi_io_num = SPI_GPIO_MOSI,
        .miso_io_num = SPI_GPIO_MISO,
        .sclk_io_num = SPI_GPIO_SCLK
    };

    //Configuration for the SPI slave interface
    static spi_slave_interface_config_t spi_slave_cfg = {
        .mode = 0,
        .spics_io_num = SPI_GPIO_CS,
        .queue_size = 2,
        .flags = 0,
        .post_setup_cb = _post_setup_cb,
        .post_trans_cb = _post_trans_cb
    };

    //Initialize SPI slave interface
    ret = spi_slave_initialize(VSPI_HOST, &spi_bus_cfg, &spi_slave_cfg, 2);
    assert(ret == ESP_OK);

    memset(&spi_tr, 0, sizeof(spi_tr));
    spi_tr.tx_buffer = buf_send;
    spi_tr.rx_buffer = buf_recv;
    spi_tr.length = SPI_SIZE_BUF * 8;

    if (eos_power_wakeup_cause()) {
        wake = 1;
        skip_msg = 1;
    }

    eos_power_wait4init();
    while (1) {
        if (!skip_msg) {
            xSemaphoreTake(mutex, portMAX_DELAY);

            eos_msgq_pop(&net_send_q, &mtype, &buffer, &len);
            if (mtype) {
                buf_send[0] = mtype;
                buf_send[1] = len >> 8;
                buf_send[2] = len & 0xFF;
                if (buffer) {
                    memcpy(buf_send + SPI_SIZE_HDR, buffer, len);
                    eos_bufq_push(&net_buf_q, buffer);
                    xSemaphoreGive(semaph);
                }
            } else {
                gpio_set_level(SPI_GPIO_RTS, 0);
                buf_send[0] = 0;
                buf_send[1] = 0;
                buf_send[2] = 0;
            }

            xSemaphoreGive(mutex);
        }
        skip_msg = 0;

        buf_recv[0] = 0;
        buf_recv[1] = 0;
        buf_recv[2] = 0;
        spi_slave_transmit(VSPI_HOST, &spi_tr, portMAX_DELAY);
        // ESP_LOGI(TAG, "RECV:%d", buf_recv[0]);

        if (wake) {
            eos_power_net_ready();
            wake = 0;
        }

        if ((spi_tr.trans_len / 8) < SPI_SIZE_HDR) continue;
        if (buf_recv[0] == 0x00) continue;

        if (buf_recv[0] == 0xFF) {  // Sleep req
            if (buf_send[0] == 0) {
                int abort = 0;

                xSemaphoreTake(mutex, portMAX_DELAY);
                net_sleep = 1;
                if (eos_msgq_len(&net_send_q)) abort = 1;
                xSemaphoreGive(mutex);

                spi_slave_free(VSPI_HOST);

                eos_power_sleep();
                if (abort) eos_power_wake(EOS_PWR_WAKE_MSG);

                vTaskSuspend(NULL);

                xSemaphoreTake(mutex, portMAX_DELAY);
                net_sleep = 0;
                xSemaphoreGive(mutex);

                spi_slave_initialize(VSPI_HOST, &spi_bus_cfg, &spi_slave_cfg, 1);
                wake = 1;
                skip_msg = 1;
            }
            continue;
        }

        mtype = buf_recv[0] & ~EOS_NET_MTYPE_FLAG_MASK;
        mtype_flags = buf_recv[0] & EOS_NET_MTYPE_FLAG_MASK;
        len   = (uint16_t)buf_recv[1] << 8;
        len  |= (uint16_t)buf_recv[2] & 0xFF;
        buffer = buf_recv + SPI_SIZE_HDR;
        if ((mtype <= EOS_NET_MAX_MTYPE) && (len <= EOS_NET_MTU)) {
            net_handler[mtype - 1](mtype, buffer, len);
        } else {
            bad_handler(mtype, buffer, len);
        }
        if ((mtype_flags & EOS_NET_MTYPE_FLAG_ONEW) && buf_send[0]) {
            skip_msg = 1;
        }
        if (mtype_flags & EOS_NET_MTYPE_FLAG_REPL) {
            spi_tr.tx_buffer = buf_recv;
            spi_tr.rx_buffer = NULL;
            spi_slave_transmit(VSPI_HOST, &spi_tr, portMAX_DELAY);
            spi_tr.tx_buffer = buf_send;
            spi_tr.rx_buffer = buf_recv;
        }
    }
    vTaskDelete(NULL);
}

void eos_net_init(void) {
    int i;

    // Configuration for the handshake lines
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    io_conf.pin_bit_mask = ((uint64_t)1 << SPI_GPIO_CTS);
    gpio_config(&io_conf);
    gpio_set_level(SPI_GPIO_CTS, 0);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    io_conf.pin_bit_mask = ((uint64_t)1 << SPI_GPIO_RTS);
    gpio_config(&io_conf);
    gpio_set_level(SPI_GPIO_RTS, 0);

    eos_msgq_init(&net_send_q, net_sndq_array, EOS_NET_SIZE_SNDQ);
    eos_bufq_init(&net_buf_q, net_bufq_array, EOS_NET_SIZE_BUFQ);
    for (i=0; i<EOS_NET_SIZE_BUFQ; i++) {
        eos_bufq_push(&net_buf_q, malloc(EOS_NET_SIZE_BUF));
    }

    for (i=0; i<EOS_NET_MAX_MTYPE; i++) {
        net_handler[i] = bad_handler;
    }

    semaph = xSemaphoreCreateCounting(EOS_NET_SIZE_BUFQ, EOS_NET_SIZE_BUFQ);
    mutex = xSemaphoreCreateBinary();
    xSemaphoreGive(mutex);
    xTaskCreate(&net_xchg_task, "net_xchg", EOS_TASK_SSIZE_NET_XCHG, NULL, EOS_TASK_PRIORITY_NET_XCHG, &net_xchg_task_handle);
    ESP_LOGI(TAG, "INIT");
}

unsigned char *eos_net_alloc(void) {
    unsigned char *ret;

    xSemaphoreTake(semaph, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    ret = eos_bufq_pop(&net_buf_q);
    xSemaphoreGive(mutex);

    return ret;
}

void eos_net_free(unsigned char *buf) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    eos_bufq_push(&net_buf_q, buf);
    xSemaphoreGive(semaph);
    xSemaphoreGive(mutex);
}

int eos_net_send(unsigned char mtype, unsigned char *buffer, uint16_t len) {
    int rv = EOS_OK;
    int sleep;

    xSemaphoreTake(mutex, portMAX_DELAY);
    sleep = net_sleep;
    gpio_set_level(SPI_GPIO_RTS, 1);
    rv = eos_msgq_push(&net_send_q, mtype, buffer, len);
    xSemaphoreGive(mutex);

    if (rv) eos_net_free(buffer);

    if (sleep) eos_power_wake(EOS_PWR_WAKE_MSG);

    return rv;
}

void eos_net_reply(unsigned char mtype, unsigned char *buffer, uint16_t len) {
    buffer -= SPI_SIZE_HDR;
    buffer[0] = mtype;
    buffer[1] = len >> 8;
    buffer[2] = len & 0xFF;
}

void eos_net_set_handler(unsigned char mtype, eos_net_fptr_t handler) {
    if (handler == NULL) handler = bad_handler;
    if (mtype && (mtype <= EOS_NET_MAX_MTYPE)) net_handler[mtype - 1] = handler;
}

void eos_net_sleep_done(uint8_t mode) {
    gpio_set_level(SPI_GPIO_CTS, 1);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(SPI_GPIO_CTS, 0);
}

void eos_net_wake(uint8_t source, uint8_t mode) {
    int sleep;

    if (mode == EOS_PWR_SMODE_DEEP) return;
    do {
        vTaskResume(net_xchg_task_handle);
        vTaskDelay(10 / portTICK_PERIOD_MS);

        xSemaphoreTake(mutex, portMAX_DELAY);
        sleep = net_sleep;
        xSemaphoreGive(mutex);
    } while (sleep);
}
