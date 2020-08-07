#include <stdlib.h>
#include <string.h>

#include "eve.h"
#include "eve_kbd.h"
#include "unicode.h"

#include "screen/screen.h"
#include "screen/window.h"
#include "screen/page.h"
#include "screen/font.h"

#include "label.h"
#include "widget.h"
#include "pagew.h"
#include "strw.h"
#include "textw.h"

static const size_t _eve_wsize[] = {
    0,
    sizeof(EVEPageWidget),
    sizeof(EVEStrWidget),
    sizeof(EVETextWidget)
};

void eve_widget_init(EVEWidget *widget, uint8_t type, EVERect *g, eve_widget_touch_t touch, eve_widget_draw_t draw, eve_kbd_input_handler_t putc) {
    if (g) widget->g = *g;
    widget->touch = touch;
    widget->draw = draw;
    widget->putc = putc;
    widget->type = type;
}

void eve_widget_set_label(EVEWidget *widget, EVELabel *label) {
    widget->label = label;
}

EVEWidget *eve_widget_next(EVEWidget *widget) {
    char *_w = (char *)widget;
    return (EVEWidget *)(_w + _eve_wsize[widget->type]);
}