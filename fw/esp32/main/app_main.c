#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/gpio.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_err.h>
#include <esp_log.h>

#include "i2c.h"
#include "cell.h"
#include "_net.h"
#include "wifi.h"
#include "sock.h"
#include "power.h"
#include "bq25895.h"

#define ESP_INTR_FLAG_DEFAULT   0

// Main application
void app_main() {
    esp_err_t ret;

    ret = esp_netif_init();
    assert(ret == ESP_OK);

    ret = esp_event_loop_create_default();
    assert(ret == ESP_OK);

    eos_net_init();

    eos_cell_pcm_init();
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    eos_modem_init();

    eos_cell_init();
    eos_wifi_init();
    eos_sock_init();

    eos_power_init();

    eos_i2c_init();
    eos_bq25895_set_ilim();
}


