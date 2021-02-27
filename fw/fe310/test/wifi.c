#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <eos.h>
#include <event.h>
#include <spi.h>
#include <uart.h>
#include <net.h>
#include <wifi.h>

#include <unicode.h>

#include <eve/eve.h>
#include <eve/eve_kbd.h>

#include <eve/screen/screen.h>
#include <eve/screen/window.h>
#include <eve/screen/view.h>
#include <eve/screen/page.h>
#include <eve/screen/form.h>

#include <eve/widget/widgets.h>

#include <app/root.h>

#include "status.h"
#include "wifi.h"

extern EVEFont *_app_font_default;

void wifi_scan_handler(unsigned char type, unsigned char *buffer, uint16_t size) {
    EVEScreen *screen = app_screen();
    EVEWindow *window = eve_window_get(screen, "main");
    EVEForm *form = (EVEForm *)window->view;
    EVESelectWidget *select = (EVESelectWidget *)eve_form_widget(form, 0);

    eve_selectw_option_set(select, buffer + 1, size - 1);
    eos_net_free(buffer, 0);

    eos_spi_dev_start(EOS_DEV_DISP);
    eve_screen_draw(app_screen());
    eos_spi_dev_stop();
}

static void wifi_connect_handler(unsigned char type, unsigned char *buffer, uint16_t size) {
    app_status_msg_set("WiFi connected", 1);
    eos_net_free(buffer, 0);
}

static void wifi_disconnect_handler(unsigned char type, unsigned char *buffer, uint16_t size) {
    app_status_msg_set("WiFi disconnected", 1);
    eos_net_free(buffer, 0);
}

void app_wifi(EVEWindow *window, EVEViewStack *stack) {
    char *title = "Password:";
    uint16_t w = eve_font_str_w(_app_font_default, title) + 10;
    APPWidgetSpec spec[] = {
        {
            .label.g.w = APP_SCREEN_W,
            .label.font = _app_font_default,
            .label.title = "Select network:",

            .widget.type = EVE_WIDGET_TYPE_SELECT,
            .widget.g.w = APP_SCREEN_W,
            .widget.spec.select.font = _app_font_default,
            .widget.spec.select.option_size = 1500,
        },
        {
            .widget.type = EVE_WIDGET_TYPE_SPACER,
            .widget.g.w = APP_SCREEN_W,
            .widget.g.h = 50,
        },
        {
            .label.g.w = w,
            .label.font = _app_font_default,
            .label.title = title,

            .widget.type = EVE_WIDGET_TYPE_STR,
            .widget.g.w = APP_SCREEN_W - w,
            .widget.spec.str.font = _app_font_default,
            .widget.spec.str.str_size = 128,
        },
    };

    EVEForm *form = app_form_create(window, stack, spec, 3, app_wifi_action, app_wifi_close);
    eos_wifi_scan();
}

void app_wifi_action(EVEForm *form) {
    EVESelectWidget *sel = (EVESelectWidget *)eve_form_widget(form, 0);
    EVEStrWidget *str = (EVEStrWidget *)eve_form_widget(form, 2);
    char *ssid = eve_selectw_option_get_select(sel);

    eos_wifi_connect(ssid, str->str);
}

void app_wifi_close(EVEForm *form) {
    app_form_destroy(form);
}

void app_wifi_init(void) {
    eos_wifi_set_handler(EOS_WIFI_MTYPE_SCAN, wifi_scan_handler);
    eos_wifi_set_handler(EOS_WIFI_MTYPE_CONNECT, wifi_connect_handler);
    eos_wifi_set_handler(EOS_WIFI_MTYPE_DISCONNECT, wifi_disconnect_handler);
}