#include <stdlib.h>
#include <string.h>

#include "clipb.h"

#include "eve.h"
#include "eve_kbd.h"
#include "eve_font.h"

#include "screen/window.h"
#include "screen/page.h"

#include "label.h"
#include "widget.h"
#include "strw.h"

#define STRW_TOUCH_OPT          EVE_TOUCH_OPT_TRACK | EVE_TOUCH_OPT_TRACK_X | EVE_TOUCH_OPT_TRACK_EXT_X | EVE_TOUCH_OPT_LPRESS

#define STRW_TMODE_NONE         0
#define STRW_TMODE_CRSR         1
#define STRW_TMODE_TXT          2

#define CH_BS                   0x08
#define CH_DEL                  0x7f

#define CH_CTRLX                0x18
#define CH_CTRLC                0x03
#define CH_CTRLV                0x16

#define CHAR_VALID_INPUT(c)     ((c >= 0x20) && (c < 0x7f))

int eve_strw_create(EVEStrWidget *widget, EVERect *g, EVEPage *page, EVEStrSpec *spec) {
    EVEFont *font = spec->font ? spec->font : eve_window_font(page->v.window);
    utf8_t *str;
    uint16_t *line;

    str = eve_malloc(spec->str_size);
    if (str == NULL) return EVE_ERR_NOMEM;
    str[0] = '\0';

    eve_strw_init(widget, g, page, font, str, spec->str_size);

    return EVE_OK;
}

void eve_strw_init(EVEStrWidget *widget, EVERect *g, EVEPage *page, EVEFont *font, utf8_t *str, uint16_t str_size) {
    EVEWidget *_widget = &widget->w;
    int rv, str_len;

    memset(widget, 0, sizeof(EVEStrWidget));
    eve_widget_init(_widget, EVE_WIDGET_TYPE_STR, g, page, eve_strw_draw, eve_strw_touch, eve_strw_putc);
    widget->font = font;
    rv = utf8_verify(str, str_size, &str_len);
    if (rv != UTF_OK) {
        if (str_len >= str_size) str_len = 0;
        str[str_len] = '\0';
    }
    widget->str = str;
    widget->str_size = str_size;
    widget->str_len = str_len;
    widget->str_g.w = eve_font_str_w(widget->font, str);
    if (_widget->g.h == 0) _widget->g.h = eve_font_h(widget->font);
}

int eve_strw_update(EVEStrWidget *widget) {
    int rv, str_len;

    rv = utf8_verify(widget->str, widget->str_size, &str_len);
    if (rv != UTF_OK) {
        if (str_len >= widget->str_size) str_len = 0;
        widget->str[str_len] = '\0';
    }
    widget->str_len = str_len;
    return (rv == UTF_OK) ? EVE_OK : EVE_ERR;
}

void eve_strw_destroy(EVEStrWidget *widget) {
    eve_free(widget->str);
}

static void set_focus(EVEStrWidget *widget) {
    EVEWidget *_widget = &widget->w;
    EVERect focus;

    focus.x = _widget->g.x;
    focus.y = _widget->g.y;
    focus.w = _widget->g.w;
    focus.h = 2 * widget->font->h;
    eve_widget_focus(_widget, &focus);
}

static EVEStrCursor *cursor_prox(EVEStrWidget *widget, EVEStrCursor *cursor, EVETouch *touch, short *dx) {
    EVEWidget *_widget = &widget->w;
    EVEPage *page = _widget->page;
    int x = eve_page_x(page, touch->x0) - _widget->g.x + widget->str_g.x;
    int _dx;

    *dx = cursor->x - x;
    _dx = *dx < 0 ? -(*dx) : *dx;

    if (_dx <= widget->font->w) return cursor;
    return NULL;
}

static void draw_string(EVEStrWidget *widget, uint16_t ch, uint16_t len, uint16_t x1, uint16_t x2, char s) {
    int16_t x;
    EVEWidget *_widget = &widget->w;
    EVEPage *page = _widget->page;

    x = _widget->g.x - widget->str_g.x;
    if (x1 != x2) {
        eve_cmd_dl(BEGIN(EVE_RECTS));
        if (!s) eve_cmd_dl(COLOR_MASK(0 ,0 ,0 ,0));
        eve_cmd_dl(VERTEX2F(x + x1, _widget->g.y));
        eve_cmd_dl(VERTEX2F(x + x2, _widget->g.y + widget->font->h));
        if (!s) {
            eve_cmd_dl(COLOR_MASK(1 ,1 ,1 ,1));
            eve_cmd_dl(BEGIN(EVE_LINES));
            eve_cmd_dl(VERTEX2F(x + x1, _widget->g.y + widget->font->h));
            eve_cmd_dl(VERTEX2F(x + x2, _widget->g.y + widget->font->h));
        }
        eve_cmd_dl(END());
        if (len) {
            if (s) eve_cmd_dl(COLOR_RGBC(page->v.color_bg));
            eve_cmd(CMD_TEXT, "hhhhpb", x + x1, _widget->g.y, widget->font->id, 0, widget->str + ch, len, 0);
            if (s) eve_cmd_dl(COLOR_RGBC(page->v.color_fg));
        }
    }
}

static void draw_cursor(EVEStrWidget *widget, EVEStrCursor *cursor) {
    uint16_t x, y;
    EVEWidget *_widget = &widget->w;

    x = _widget->g.x - widget->str_g.x + cursor->x;
    y = _widget->g.y;
    eve_cmd_dl(BEGIN(EVE_LINES));
    eve_cmd_dl(VERTEX2F(x, y));
    eve_cmd_dl(VERTEX2F(x, y + widget->font->h));
    eve_cmd_dl(END());
}

uint8_t eve_strw_draw(EVEWidget *_widget, uint8_t tag0) {
    EVEStrWidget *widget = (EVEStrWidget *)_widget;
    char cut = widget->str_g.x || (widget->str_g.w > _widget->g.w);

    _widget->tag0 = tag0;
    if (tag0 != EVE_NOTAG) {
        eve_cmd_dl(TAG(tag0));
        eve_touch_set_opt(tag0, STRW_TOUCH_OPT);
        tag0++;
    }
    _widget->tagN = tag0;

    if (cut) {
        EVEPage *page = _widget->page;
        EVEWindow *window = page->v.window;
        int16_t x = eve_page_scr_x(page, _widget->g.x);
        int16_t y = eve_page_scr_y(page, _widget->g.y);
        uint16_t w = _widget->g.w;
        uint16_t h = _widget->g.h;
        int16_t win_x1 = window->g.x;
        int16_t win_y1 = window->g.y;
        int16_t win_x2 = win_x1 + window->g.w;
        int16_t win_y2 = win_y1 + window->g.h;

        if (win_x1 < 0) win_x1 = 0;
        if (win_y1 < 0) win_y1 = 0;
        if (win_x2 > window->root->w.g.w) win_x2 = window->root->w.g.w;
        if (win_y2 > window->root->w.g.h) win_y2 = window->root->w.g.h;
        if (x < win_x1) {
            w += x - win_x1;
            x = win_x1;
        }
        if (y < win_y1) {
            h += y - win_y1;
            y = win_y1;
        }
        if (x + w > win_x2) w = win_x2 - x;
        if (y + h > win_y2) h = win_y2 - y;

        eve_cmd_dl(SAVE_CONTEXT());
        eve_cmd_dl(SCISSOR_XY(x, y));
        eve_cmd_dl(SCISSOR_SIZE(w, h));
    }

    if (widget->cursor2.on) {
        EVEStrCursor *c1, *c2;
        int l1, l2, l3;

        if (widget->cursor1.ch <= widget->cursor2.ch) {
            c1 = &widget->cursor1;
            c2 = &widget->cursor2;
        } else {
            c1 = &widget->cursor2;
            c2 = &widget->cursor1;
        }

        l1 = c1->ch;
        l2 = c2->ch - c1->ch;
        l3 = widget->str_len - c2->ch;
        draw_string(widget, 0, l1, 0, c1->x, 0);
        draw_string(widget, c1->ch, l2, c1->x, c2->x, 1);
        draw_string(widget, c2->ch, l3, c2->x, widget->str_g.x + _widget->g.w, 0);
    } else {
        if (widget->cursor1.on) draw_cursor(widget, &widget->cursor1);
        draw_string(widget, 0, widget->str_len, 0, widget->str_g.x + _widget->g.w, 0);
    }

    if (cut) {
        eve_cmd_dl(RESTORE_CONTEXT());
    }

    return _widget->tagN;
}

int eve_strw_touch(EVEWidget *_widget, EVETouch *touch, uint16_t evt) {
    EVEPage *page = _widget->page;
    EVEStrWidget *widget = (EVEStrWidget *)_widget;
    EVEStrCursor *t_cursor = NULL;
    short dx;
    int ret = 0;

    if (evt & (EVE_TOUCH_ETYPE_LPRESS | EVE_TOUCH_ETYPE_TRACK_START)) {
        if (widget->cursor2.on) {
            t_cursor = cursor_prox(widget, &widget->cursor2, touch, &dx);
        }
        if ((t_cursor == NULL) && widget->cursor1.on) {
            t_cursor = cursor_prox(widget, &widget->cursor1, touch, &dx);
        }
        if (evt & EVE_TOUCH_ETYPE_TRACK_START) {
            if (t_cursor) {
                widget->track.mode = STRW_TMODE_CRSR;
                widget->track.cursor = t_cursor;
                widget->track.dx = dx;
            } else if (touch->eevt & EVE_TOUCH_EETYPE_TRACK_X) {
                widget->track.mode = STRW_TMODE_TXT;
            }
        }
    }

    if (widget->track.mode) {
        int x, w1;

        if (evt & EVE_TOUCH_ETYPE_TRACK) {
            switch (widget->track.mode) {
                case STRW_TMODE_TXT:
                    if (evt & EVE_TOUCH_ETYPE_TRACK_START) {
                        widget->str_g.x0 = widget->str_g.x;
                    }
                    x = widget->str_g.x0 + touch->x0 - touch->x;
                    w1 = _widget->g.w - widget->font->w;
                    if (x > widget->str_g.w - w1) x = widget->str_g.w - w1;
                    if (x < 0) x = 0;
                    widget->str_g.x = x;
                    break;

                case STRW_TMODE_CRSR:
                    eve_strw_cursor_set(widget, widget->track.cursor, eve_page_x(page, touch->x) + widget->track.dx);
                    break;
            }
        }
        ret = 1;
    } else {
        if (evt & EVE_TOUCH_ETYPE_LPRESS) {
            if (widget->cursor2.on) {
                // copy
            } else if (widget->cursor1.on) {
                if (t_cursor) {
                    // paste
                } else {
                    eve_strw_cursor_set(widget, &widget->cursor2, eve_page_x(page, touch->x));
                }
            } else {
                // select
            }
            ret = 1;
        }
        if ((evt & EVE_TOUCH_ETYPE_POINT_UP) && !(touch->eevt & (EVE_TOUCH_EETYPE_TRACK_XY | EVE_TOUCH_EETYPE_ABORT | EVE_TOUCH_EETYPE_LPRESS))) {
            eve_strw_cursor_set(widget, &widget->cursor1, eve_page_x(page, touch->x0));
            if (widget->cursor2.on) eve_strw_cursor_clear(widget, &widget->cursor2);
            set_focus(widget);
            ret = 1;
        }
    }

    if (evt & EVE_TOUCH_ETYPE_TRACK_STOP) {
        widget->track.mode = STRW_TMODE_NONE;
        widget->track.cursor = NULL;
        widget->track.dx = 0;
    }

    return ret;
}

void eve_strw_putc(void *w, int c) {
    EVEStrWidget *widget = (EVEStrWidget *)w;
    EVEWidget *_widget = &widget->w;
    EVEStrCursor *cursor1 = &widget->cursor1;
    EVEStrCursor *cursor2 = &widget->cursor2;
    utf8_t *str;
    utf8_t *clipb = NULL;
    int w0 = widget->font->w;
    int w1 = _widget->g.w - widget->font->w;
    int ins_c = 0, del_c = 0;
    int ins_w = 0, del_w = 0;


    if (c == EVE_PAGE_KBDCH_CLOSE) {
        if (cursor1->on) eve_strw_cursor_clear(widget, cursor1);
        if (cursor2->on) eve_strw_cursor_clear(widget, cursor2);
        return;
    }

    if (!cursor1->on) return;

    if (!cursor2->on && ((c == CH_BS) || (c == CH_DEL))) {
        utf32_t uc;

        str = widget->str + cursor1->ch;
        switch (c) {
            case CH_BS:
                if (cursor1->ch > 0) {
                    del_c = -utf8_seek(str, -1, &uc);
                    del_w = eve_font_ch_w(widget->font, uc);
                    memmove(str - del_c, str, widget->str_len - cursor1->ch + 1);
                    widget->str_len -= del_c;
                    widget->str_g.w -= del_w;
                    cursor1->ch -= del_c;
                    cursor1->x -= del_w;
                }
                break;

            case CH_DEL:
                if (cursor1->ch < widget->str_len) {
                    del_c = utf8_dec(str, &uc);
                    del_w = eve_font_ch_w(widget->font, uc);
                    memmove(str, str + del_c, widget->str_len - cursor1->ch - del_c + 1);
                    widget->str_len -= del_c;
                    widget->str_g.w -= del_w;
                }
                break;
        }
        if (widget->str_g.w - widget->str_g.x < w1) {
            widget->str_g.x -= del_w;
            if (widget->str_g.x < 0) widget->str_g.x = 0;
        }
    } else {
        EVEStrCursor *c1 = cursor1;
        EVEStrCursor *c2 = cursor1;
        utf8_t utf8_buf[4];

        if (cursor2->on) {
            if (cursor1->ch <= cursor2->ch) {
                c2 = cursor2;
            } else {
                c1 = cursor2;
            }
            del_c = c2->ch - c1->ch;
            del_w = eve_font_buf_w(widget->font, str, del_c);
            if ((c == CH_CTRLX) || (c == CH_CTRLC)) {
                eve_clipb_push(str, del_c);
                if (c == CH_CTRLC) return;
            }
        }

        str = widget->str + c1->ch;
        if (CHAR_VALID_INPUT(c)) {
            ins_c = utf8_enc(c, utf8_buf);
            ins_w = eve_font_ch_w(widget->font, c);
        } else if (c == CH_CTRLV) {
            int rv, clipb_len = 0;

            clipb = eve_clipb_get();
            if (clipb) {
                rv = utf8_verify(clipb, EVE_CLIPB_SIZE_BUF, &clipb_len);
                if (rv != UTF_OK) {
                    clipb = NULL;
                    clipb_len = 0;
                }
            }
            ins_c = clipb_len;
            ins_w = eve_font_str_w(widget->font, clipb);
        }
        if (widget->str_len + ins_c >= widget->str_size + del_c) {
            ins_c = 0;
            ins_w = 0;
        }
        if (ins_c != del_c) memmove(str + ins_c, str + del_c, widget->str_len - c2->ch + 1);
        if (ins_c) {
            if (c == CH_CTRLV) {
                memcpy(str, clipb, ins_c);
            } else if (ins_c > 1) {
                memcpy(str, utf8_buf, ins_c);
            } else {
                *str = utf8_buf[0];
            }
            c1->ch += ins_c;
            c1->x += ins_w;
        }
        widget->str_len += ins_c - del_c;
        widget->str_g.w += ins_w - del_w;
        if (c1 == cursor2) widget->cursor1 = widget->cursor2;
        if (cursor2->on) eve_strw_cursor_clear(widget, cursor2);
    }

    if (cursor1->x - widget->str_g.x < w0) widget->str_g.x = cursor1->x > w0 ? cursor1->x - w0 : 0;
    if (cursor1->x - widget->str_g.x > w1) widget->str_g.x = cursor1->x - w1;
}

void eve_strw_cursor_set(EVEStrWidget *widget, EVEStrCursor *cursor, int16_t x) {
    int i;
    int16_t _x, _d;
    utf32_t ch;
    uint8_t ch_w;
    uint8_t ch_l;
    EVEWidget *_widget = &widget->w;

    x = x - _widget->g.x + widget->str_g.x;

    _x = 0;
    _d = x;
    i = 0;
    while (i < widget->str_len) {
        ch_l = utf8_dec(widget->str + i, &ch);
        ch_w = eve_font_ch_w(widget->font, ch);
        _x += ch_w;
        i += ch_l;
        if (_x >= x) {
            if (_x - x > _d) {
                _x -= ch_w;
                i -= ch_l;
            }
            break;
        } else {
            _d = x - _x;
        }
    }
    cursor->x = _x;
    cursor->ch = i;
    cursor->on = 1;
}

void eve_strw_cursor_clear(EVEStrWidget *widget, EVEStrCursor *cursor) {
    cursor->on = 0;
}
