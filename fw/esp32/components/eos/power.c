#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_pm.h>
#include <esp_log.h>

#include "eos.h"
#include "net.h"
#include "cell.h"
#include "power.h"

#define POWER_GPIO_BTN      0
#define POWER_GPIO_NET      5
#define POWER_GPIO_UART     35

#define POWER_ETYPE_BTN     1
#define POWER_ETYPE_SLEEP   2
#define POWER_ETYPE_WAKE    3
#define POWER_ETYPE_NETRDY  4

typedef struct {
    uint8_t type;
    union {
        uint8_t source;
        uint8_t level;
    };
} power_event_t;

static esp_pm_lock_handle_t power_lock_cpu_freq;
static esp_pm_lock_handle_t power_lock_apb_freq;
static esp_pm_lock_handle_t power_lock_no_sleep;

static const char *TAG = "EOS POWER";

static QueueHandle_t power_queue;

static volatile int init_done = 0;

static void IRAM_ATTR btn_handler(void *arg) {
    power_event_t evt;

    evt.type = POWER_ETYPE_BTN;
    evt.level = gpio_get_level(POWER_GPIO_BTN);
    xQueueSendFromISR(power_queue, &evt, NULL);
}

static void IRAM_ATTR btn_wake_handler(void *arg) {
    power_event_t evt;

    gpio_intr_disable(POWER_GPIO_BTN);

    evt.type = POWER_ETYPE_WAKE;
    evt.source = EOS_PWR_WAKE_BTN;
    xQueueSendFromISR(power_queue, &evt, NULL);

}

static void IRAM_ATTR net_wake_handler(void *arg) {
    power_event_t evt;

    gpio_intr_disable(POWER_GPIO_NET);

    evt.type = POWER_ETYPE_WAKE;
    evt.source = EOS_PWR_WAKE_NET;
    xQueueSendFromISR(power_queue, &evt, NULL);
}

static void IRAM_ATTR uart_wake_handler(void *arg) {
    power_event_t evt;

    gpio_intr_disable(POWER_GPIO_UART);

    evt.type = POWER_ETYPE_WAKE;
    evt.source = EOS_PWR_WAKE_UART;
    xQueueSendFromISR(power_queue, &evt, NULL);
}

void power_sleep(uint8_t mode) {
    gpio_config_t io_conf;

    eos_modem_sleep(mode);
    eos_net_sleep_done(mode);

    switch (mode) {
        case EOS_PWR_SMODE_LIGHT:
            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pin_bit_mask = ((uint64_t)1 << POWER_GPIO_NET);
            io_conf.pull_up_en = 0;
            io_conf.pull_down_en = 0;
            gpio_config(&io_conf);

            gpio_isr_handler_add(POWER_GPIO_BTN, btn_wake_handler, NULL);
            gpio_isr_handler_add(POWER_GPIO_NET, net_wake_handler, NULL);
            gpio_isr_handler_add(POWER_GPIO_UART, uart_wake_handler, NULL);

            esp_sleep_enable_gpio_wakeup();
            gpio_wakeup_enable(POWER_GPIO_BTN, GPIO_INTR_LOW_LEVEL);
            gpio_wakeup_enable(POWER_GPIO_NET, GPIO_INTR_LOW_LEVEL);
            gpio_wakeup_enable(POWER_GPIO_UART, GPIO_INTR_LOW_LEVEL);

            ESP_LOGI(TAG, "SLEEP");

            esp_pm_lock_release(power_lock_apb_freq);
            esp_pm_lock_release(power_lock_no_sleep);

            break;

        case EOS_PWR_SMODE_DEEP:
            gpio_deep_sleep_hold_en();
            esp_sleep_enable_ext0_wakeup(POWER_GPIO_BTN, 0);
            esp_sleep_enable_ext1_wakeup((uint64_t)1 << POWER_GPIO_UART, ESP_EXT1_WAKEUP_ALL_LOW);

            ESP_LOGI(TAG, "SLEEP");

            esp_deep_sleep_start();
            break;

        default:
            break;
    }
}

void power_wake_stage1(uint8_t source, uint8_t mode) {
    gpio_config_t io_conf;

    if (mode == EOS_PWR_SMODE_LIGHT) {
        esp_pm_lock_acquire(power_lock_apb_freq);
        esp_pm_lock_acquire(power_lock_no_sleep);

        gpio_wakeup_disable(POWER_GPIO_BTN);
        gpio_wakeup_disable(POWER_GPIO_NET);
        gpio_wakeup_disable(POWER_GPIO_UART);

        gpio_isr_handler_remove(POWER_GPIO_NET);
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_DISABLE;
        io_conf.pin_bit_mask = ((uint64_t)1 << POWER_GPIO_NET);
        io_conf.pull_up_en = 0;
        io_conf.pull_down_en = 0;
        gpio_config(&io_conf);
    }

    gpio_intr_disable(POWER_GPIO_BTN);
    if ((source != EOS_PWR_WAKE_BTN) && (source != EOS_PWR_WAKE_NET)) {
        gpio_set_direction(POWER_GPIO_BTN, GPIO_MODE_OUTPUT);
        gpio_set_level(POWER_GPIO_BTN, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_direction(POWER_GPIO_BTN, GPIO_MODE_INPUT);
    }

    eos_net_wake(source, mode);
}

void power_wake_stage2(uint8_t source, uint8_t mode) {
    eos_modem_wake(source, mode);

    gpio_set_intr_type(POWER_GPIO_BTN, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(POWER_GPIO_BTN, btn_handler, NULL);

    ESP_LOGI(TAG, "WAKE");
}

static void power_event_task(void *pvParameters) {
    unsigned char *buf;
    power_event_t evt;
    uint8_t source;
    uint8_t wakeup_cause;
    uint8_t mode;
    int sleep;

    source = 0;
    wakeup_cause = eos_power_wakeup_cause();
    if (wakeup_cause) {
        mode = EOS_PWR_SMODE_DEEP;
        sleep = 1;
    } else {
        mode = EOS_PWR_SMODE_LIGHT;
        sleep = 0;
    }

    while (1) {
        if (xQueueReceive(power_queue, &evt, portMAX_DELAY)) {
            switch (evt.type) {
                case POWER_ETYPE_SLEEP:
                    if (!sleep) {
                        mode = EOS_PWR_SMODE_DEEP;
                        power_sleep(mode);
                        sleep = 1;
                    }
                    break;

                case POWER_ETYPE_WAKE:
                    if (sleep) {
                        source = evt.source;
                        power_wake_stage1(source, mode);
                    }
                    break;

                case POWER_ETYPE_NETRDY:
                    if (sleep && source) {
                        power_wake_stage2(source, mode);
                        sleep = 0;
                        source = 0;
                    }
                    break;

                case POWER_ETYPE_BTN:
                    buf = eos_net_alloc();
                    buf[0] = EOS_PWR_MTYPE_BUTTON;
                    buf[1] = evt.level;
                    eos_net_send(EOS_NET_MTYPE_POWER, buf, 2);
                    break;

                default:
                    break;
            }
        }
    }
    vTaskDelete(NULL);
}

void eos_power_init(void) {
    esp_err_t ret;
    gpio_config_t io_conf;
    esp_pm_config_esp32_t pwr_conf;
    uint8_t wakeup_cause;

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = ((uint64_t)1 << POWER_GPIO_BTN);
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
    gpio_isr_handler_add(POWER_GPIO_BTN, btn_handler, NULL);

    /*
    ret = esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    assert(ret == ESP_OK);
    ret = esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    assert(ret == ESP_OK);
    ret = esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    assert(ret == ESP_OK);
    */

    ret = esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, NULL, &power_lock_cpu_freq);
    assert(ret == ESP_OK);
    ret = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, NULL, &power_lock_apb_freq);
    assert(ret == ESP_OK);
    ret = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, NULL, &power_lock_no_sleep);
    assert(ret == ESP_OK);

    ret = esp_pm_lock_acquire(power_lock_cpu_freq);
    assert(ret == ESP_OK);
    ret = esp_pm_lock_acquire(power_lock_apb_freq);
    assert(ret == ESP_OK);
    ret = esp_pm_lock_acquire(power_lock_no_sleep);
    assert(ret == ESP_OK);

    pwr_conf.max_freq_mhz = 160;
    pwr_conf.min_freq_mhz = 80;
    pwr_conf.light_sleep_enable = 1;

    ret = esp_pm_configure(&pwr_conf);
    assert(ret == ESP_OK);

    power_queue = xQueueCreate(4, sizeof(power_event_t));
    xTaskCreate(power_event_task, "power_event", EOS_TASK_SSIZE_PWR, NULL, EOS_TASK_PRIORITY_PWR, NULL);

    wakeup_cause = eos_power_wakeup_cause();
    if (wakeup_cause) eos_power_wake(wakeup_cause);

    init_done = 1;
    ESP_LOGI(TAG, "INIT");
}

void eos_power_wait4init(void) {
    while (!init_done);
}

uint8_t eos_power_wakeup_cause(void) {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            return EOS_PWR_WAKE_BTN;

        case ESP_SLEEP_WAKEUP_EXT1:
            return EOS_PWR_WAKE_UART;

        default:
            return EOS_PWR_WAKE_RST;
    }
}

void eos_power_sleep(void) {
    power_event_t evt;

    evt.type = POWER_ETYPE_SLEEP;
    evt.source = 0;
    xQueueSend(power_queue, &evt, portMAX_DELAY);
}

void eos_power_wake(uint8_t source) {
    power_event_t evt;

    evt.type = POWER_ETYPE_WAKE;
    evt.source = source;

    xQueueSend(power_queue, &evt, portMAX_DELAY);
}

void eos_power_net_ready(void) {
    power_event_t evt;

    evt.type = POWER_ETYPE_NETRDY;
    evt.source = 0;

    xQueueSend(power_queue, &evt, portMAX_DELAY);
}
