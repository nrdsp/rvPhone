#include <stdlib.h>
#include <string.h>

#include "eve.h"
#include "eve_kbd.h"
#include "unicode.h"

#include "screen/screen.h"
#include "screen/window.h"
#include "screen/view.h"
#include "screen/page.h"

#include "font.h"
#include "label.h"
#include "widget.h"
#include "pagew.h"

int eve_pagew_create(EVEPageWidget *widget, EVERect *g, EVEPageSpec *spec) {
    eve_pagew_init(widget, g, spec->font, spec->title, spec->constructor);

    return EVE_OK;
}

void eve_pagew_init(EVEPageWidget *widget, EVERect *g, EVEFont *font, char *title, eve_view_constructor_t constructor) {
    EVEWidget *_widget = &widget->w;

    memset(widget, 0, sizeof(EVEPageWidget));
    eve_widget_init(_widget, EVE_WIDGET_TYPE_PAGE, g, eve_pagew_touch, eve_pagew_draw, NULL);
    eve_pagew_update(widget, font, title, constructor);
}

void eve_pagew_update(EVEPageWidget *widget, EVEFont *font, char *title, eve_view_constructor_t constructor) {
    EVEWidget *_widget = &widget->w;

    if (font) widget->font = font;
    if (title) widget->title = title;
    if (constructor) widget->constructor = constructor;
    if (_widget->g.w == 0) _widget->g.w = eve_font_str_w(font, widget->title);
    if (_widget->g.h == 0) _widget->g.h = eve_font_h(font);
}

int eve_pagew_touch(EVEWidget *_widget, EVEPage *page, EVETouch *t, uint16_t evt) {
    EVEPageWidget *widget = (EVEPageWidget *)_widget;

    if (evt & EVE_TOUCH_ETYPE_TAG_UP) {
        eve_page_open(page, widget->constructor);
        return 1;
    }

    return 0;
}

uint8_t eve_pagew_draw(EVEWidget *_widget, EVEPage *page, uint8_t tag0) {
    EVEPageWidget *widget = (EVEPageWidget *)_widget;

    _widget->tag0 = tag0;
    if (tag0 != EVE_TAG_NOTAG) {
        eve_cmd_dl(TAG(tag0));
        tag0++;
    }
    _widget->tagN = tag0;

    eve_cmd(CMD_TEXT, "hhhhs", _widget->g.x, _widget->g.y, widget->font->id, 0, widget->title);

    return _widget->tagN;
}
