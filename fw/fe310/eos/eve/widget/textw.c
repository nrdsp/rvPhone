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
#include "textw.h"

#define TEXTW_TOUCH_OPT         EVE_TOUCH_OPT_TRACK | EVE_TOUCH_OPT_TRACK_XY | EVE_TOUCH_OPT_TRACK_EXT_XY | EVE_TOUCH_OPT_LPRESS

#define CH_BS                   0x08
#define CH_DEL                  0x7f

#define CH_CTRLX                0x18
#define CH_CTRLC                0x03
#define CH_CTRLV                0x16

#define LINE_LEN(w,l)           ((l) ? (w->line[l] ? w->line[l] - w->line[(l) - 1] - 1 : 0) : w->line[l])
#define LINE_START(w,l)         ((l) ? w->line[(l) - 1] + 1 : 0)
#define LINE_END(w,l)           (w->line[l])
#define LINE_EMPTY              0xffff

#define CHAR_VALID_INPUT(c)     (((c >= 0x20) && (c < 0x7f)) || (c == '\n'))

#define DIVC(x,y)               ((x) / (y) + ((x) % (y) != 0))

int eve_textw_create(EVETextWidget *widget, EVERect *g, EVEPage *page, EVETextSpec *spec) {
    EVEFont *font = spec->font ? spec->font : eve_window_font(page->v.window);
    utf8_t *text;
    uint16_t *line;

    text = eve_malloc(spec->text_size);
    if (text == NULL) {
        return EVE_ERR_NOMEM;
    }
    text[0] = '\0';
    line = eve_malloc(sizeof(uint16_t) * spec->line_size);
    if (line == NULL) {
        free(text);
        return EVE_ERR_NOMEM;
    }

    eve_textw_init(widget, g, page, font, text, spec->text_size, line, spec->line_size);

    return EVE_OK;
}

void eve_textw_init(EVETextWidget *widget, EVERect *g, EVEPage *page, EVEFont *font, utf8_t *text, uint16_t text_size, uint16_t *line, uint16_t line_size) {
    EVEWidget *_widget = &widget->w;
    int rv, text_len;

    memset(widget, 0, sizeof(EVETextWidget));
    eve_widget_init(_widget, EVE_WIDGET_TYPE_TEXT, g, page, eve_textw_draw, eve_textw_touch, eve_textw_putc);
    widget->font = font;
    rv = utf8_verify(text, text_size, &text_len);
    if (rv != UTF_OK) {
        if (text_len >= text_size) text_len = 0;
        text[text_len] = '\0';
    }
    widget->text = text;
    widget->text_size = text_size;
    widget->text_len = text_len;
    widget->line = line;
    widget->line_size = line_size;
    memset(widget->line, 0xff, line_size * sizeof(uint16_t));
    eve_textw_text_update(widget, 0, 0);
}

void eve_textw_destroy(EVETextWidget *widget) {
    eve_free(widget->line);
    eve_free(widget->text);
}

static void set_focus(EVETextWidget *widget, EVETextCursor *cursor) {
    EVEWidget *_widget = &widget->w;
    EVERect focus;

    focus.x = _widget->g.x;
    focus.y = _widget->g.y + cursor->line * widget->font->h;
    focus.w = _widget->g.w;
    focus.h = 2 * widget->font->h;
    eve_widget_focus(_widget, &focus);
}

static EVETextCursor *cursor_prox(EVETextWidget *widget, EVETextCursor *cursor, EVETouch *touch, short *dx, short *dl) {
    EVEWidget *_widget = &widget->w;
    EVEPage *page = _widget->page;
    int x = eve_page_x(page, touch->x0) - _widget->g.x;
    int l = (int)touch->tag0 - _widget->tag0 + widget->line0;
    int _dx, _dl;

    *dx = cursor->x - x;
    *dl = cursor->line - l;

    _dx = *dx < 0 ? -(*dx) : *dx;
    _dl = *dl < 0 ? -(*dl) : *dl;

    if ((_dx <= widget->font->h) && (_dl <= 1)) return cursor;
    return NULL;
}

static void draw_line(EVETextWidget *widget, uint16_t l, uint16_t ch, uint16_t len, uint16_t x1, uint16_t x2, char s) {
    EVEWidget *_widget = &widget->w;
    EVEPage *page = _widget->page;

    if (x1 != x2) {
        eve_cmd_dl(BEGIN(EVE_RECTS));
        if (!s) eve_cmd_dl(COLOR_MASK(0 ,0 ,0 ,0));
        eve_cmd_dl(VERTEX2F(_widget->g.x + x1, _widget->g.y + l * widget->font->h));
        eve_cmd_dl(VERTEX2F(_widget->g.x + x2, _widget->g.y + (l + 1) * widget->font->h));
        if (!s) {
            eve_cmd_dl(COLOR_MASK(1 ,1 ,1 ,1));
            eve_cmd_dl(BEGIN(EVE_LINES));
            eve_cmd_dl(VERTEX2F(_widget->g.x + x1, _widget->g.y + (l + 1) * widget->font->h));
            eve_cmd_dl(VERTEX2F(_widget->g.x + x2, _widget->g.y + (l + 1) * widget->font->h));
        }
        eve_cmd_dl(END());
        if (len) {
            if (s) eve_cmd_dl(COLOR_RGBC(page->v.color_bg));
            eve_cmd(CMD_TEXT, "hhhhpb", _widget->g.x + x1, _widget->g.y + l * widget->font->h, widget->font->id, 0, widget->text + ch, len, 0);
            if (s) eve_cmd_dl(COLOR_RGBC(page->v.color_fg));
        }
    }
}

static void draw_cursor(EVETextWidget *widget, EVETextCursor *cursor) {
    uint16_t x, y;
    EVEWidget *_widget = &widget->w;

    x = _widget->g.x + cursor->x;
    y = _widget->g.y + cursor->line * widget->font->h;
    eve_cmd_dl(BEGIN(EVE_LINES));
    eve_cmd_dl(VERTEX2F(x, y));
    eve_cmd_dl(VERTEX2F(x, y + widget->font->h));
    eve_cmd_dl(END());
}

uint8_t eve_textw_draw(EVEWidget *_widget, uint8_t tag0) {
    EVEPage *page = _widget->page;
    EVETextWidget *widget = (EVETextWidget *)_widget;
    int line0, lineN;
    int _line0, _lineN;
    char lineNvisible;

    _line0 = line0 = -((int)eve_page_win_y(page, _widget->g.y)) / widget->font->h;
    _lineN = lineN = DIVC((-((int)eve_page_win_y(page, _widget->g.y)) + page->v.window->g.h), widget->font->h);
    if (line0 < 0) line0 = 0;
    if (lineN < 0) lineN = 0;
    if (line0 > widget->line_len) line0 = widget->line_len;
    if (lineN > widget->line_len) lineN = widget->line_len;
    lineNvisible = (lineN >= _line0) && (lineN < _lineN);

    if (lineNvisible || (line0 < lineN)) {
        int i;
        char s = 0;
        EVETextCursor *c1, *c2;

        widget->line0 = line0;
        _widget->tag0 = tag0;
        _widget->tagN = tag0;

        if (widget->cursor2.on) {
            if (widget->cursor1.ch <= widget->cursor2.ch) {
                c1 = &widget->cursor1;
                c2 = &widget->cursor2;
            } else {
                c1 = &widget->cursor2;
                c2 = &widget->cursor1;
            }
        } else {
            c1 = NULL;
            c2 = NULL;
        }

        for (i=line0; i<lineN; i++) {
            if (_widget->tagN != EVE_NOTAG) {
                eve_cmd_dl(TAG(_widget->tagN));
                eve_touch_set_opt(_widget->tagN, TEXTW_TOUCH_OPT);
                _widget->tagN++;
            }
            if (!s && c1 && (c1->line == i)) {
                int l1, l2, l3;

                l1 = c1->ch - LINE_START(widget, i);
                if (c2->line == i) {
                    l2 = c2->ch - c1->ch;
                    l3 = LINE_START(widget, i) + LINE_LEN(widget, i) - c2->ch;
                } else {
                    l2 = LINE_START(widget, i) + LINE_LEN(widget, i) - c1->ch;
                    l3 = 0;
                    s = 1;
                }
                draw_line(widget, i, LINE_START(widget, i), l1, 0, c1->x, 0);
                draw_line(widget, i, c1->ch, l2, c1->x, s ? _widget->g.w : c2->x, 1);
                if (!s) {
                    draw_line(widget, i, c2->ch, l3, c2->x, _widget->g.w, 0);
                    c1 = NULL;
                    c2 = NULL;
                }
            } else if (s && (c2->line == i)) {
                int l1 = c2->ch - LINE_START(widget, i);
                int l2 = LINE_START(widget, i) + LINE_LEN(widget, i) - c2->ch;

                draw_line(widget, i, LINE_START(widget, i), l1, 0, c2->x, 1);
                draw_line(widget, i, c2->ch, l2, c2->x, _widget->g.w, 0);
                c1 = NULL;
                c2 = NULL;
                s = 0;
            } else {
                if (widget->cursor1.on && (widget->cursor1.line == i)) draw_cursor(widget, &widget->cursor1);
                draw_line(widget, i, LINE_START(widget, i), LINE_LEN(widget, i), 0, _widget->g.w, s);
            }
        }
        if (lineNvisible) {
            if (_widget->tagN != EVE_NOTAG) {
                eve_cmd_dl(TAG(_widget->tagN));
                eve_touch_set_opt(_widget->tagN, TEXTW_TOUCH_OPT);
                _widget->tagN++;
            }
            draw_line(widget, lineN, 0, 0, 0, _widget->g.w, 0);
        }
    } else {
        widget->line0 = 0;
        _widget->tag0 = EVE_NOTAG;
        _widget->tagN = EVE_NOTAG;
    }

    return _widget->tagN;
}

int eve_textw_touch(EVEWidget *_widget, EVETouch *touch, uint16_t evt) {
    EVEPage *page = _widget->page;
    EVETextWidget *widget = (EVETextWidget *)_widget;
    EVETextCursor *t_cursor = NULL;
    short dx, dl;
    int ret = 0;

    if (evt & (EVE_TOUCH_ETYPE_LPRESS | EVE_TOUCH_ETYPE_TRACK_START)) {
        if (widget->cursor2.on) {
            t_cursor = cursor_prox(widget, &widget->cursor2, touch, &dx, &dl);
        }
        if ((t_cursor == NULL) && widget->cursor1.on) {
            t_cursor = cursor_prox(widget, &widget->cursor1, touch, &dx, &dl);
        }
        if (t_cursor && (evt & EVE_TOUCH_ETYPE_TRACK_START)) {
            widget->track.cursor = t_cursor;
            widget->track.dx = dx;
            widget->track.dl = dl;
        }
    }

    if (widget->track.cursor) {
        if (evt & EVE_TOUCH_ETYPE_TRACK) eve_textw_cursor_set(widget, widget->track.cursor, touch->tag + widget->track.dl, eve_page_x(page, touch->x) + widget->track.dx);
        ret = 1;
    } else {
        if (evt & EVE_TOUCH_ETYPE_LPRESS) {
            if (widget->cursor2.on) {
                // copy
            } else if (widget->cursor1.on) {
                if (t_cursor && (dl == 0)) {
                    // paste
                } else {
                    eve_textw_cursor_set(widget, &widget->cursor2, touch->tag, eve_page_x(page, touch->x));
                }
            } else {
                // select
            }
            ret = 1;
        }
        if ((evt & EVE_TOUCH_ETYPE_POINT_UP) && !(touch->eevt & (EVE_TOUCH_EETYPE_TRACK_XY | EVE_TOUCH_EETYPE_ABORT | EVE_TOUCH_EETYPE_LPRESS))) {
            eve_textw_cursor_set(widget, &widget->cursor1, touch->tag_up, eve_page_x(page, touch->x0));
            if (widget->cursor2.on) eve_textw_cursor_clear(widget, &widget->cursor2);
            set_focus(widget, &widget->cursor1);
            ret = 1;
        }
    }

    if (evt & EVE_TOUCH_ETYPE_TRACK_STOP) {
        widget->track.cursor = NULL;
        widget->track.dx = 0;
        widget->track.dl = 0;
    }

    return ret;
}

void eve_textw_putc(void *w, int c) {
    EVETextWidget *widget = (EVETextWidget *)w;
    EVEWidget *_widget = &widget->w;
    EVETextCursor *cursor1 = &widget->cursor1;
    EVETextCursor *cursor2 = &widget->cursor2;
    utf8_t *text;
    utf8_t *clipb = NULL;
    int i, r;
    int ins_c = 0, del_c = 0;
    int ch_w = 0;

    if (c == EVE_PAGE_KBDCH_CLOSE) {
        if (cursor1->on) eve_textw_cursor_clear(widget, cursor1);
        if (cursor2->on) eve_textw_cursor_clear(widget, cursor2);
        return;
    }

    if (!cursor1->on) return;

    if (!cursor2->on && ((c == CH_BS) || (c == CH_DEL))) {
        utf32_t uc;

        text = widget->text + cursor1->ch;
        switch (c) {
            case CH_BS:
                if (cursor1->ch > 0) {
                    del_c = -utf8_seek(text, -1, &uc);
                    ch_w = eve_font_ch_w(widget->font, uc);
                    memmove(text - del_c, text, widget->text_len - cursor1->ch + 1);
                    cursor1->ch -= del_c;
                }
                break;

            case CH_DEL:
                if (cursor1->ch < widget->text_len) {
                    del_c = utf8_dec(text, &uc);
                    ch_w = eve_font_ch_w(widget->font, uc);
                    memmove(text, text + del_c, widget->text_len - cursor1->ch - del_c + 1);
                }
                break;
        }
    } else {
        EVETextCursor *c1 = cursor1;
        EVETextCursor *c2 = cursor1;
        utf8_t utf8_buf[4];

        if (cursor2->on) {
            if (cursor1->ch <= cursor2->ch) {
                c2 = cursor2;
            } else {
                c1 = cursor2;
            }
            del_c = c2->ch - c1->ch;
            if ((c == CH_CTRLX) || (c == CH_CTRLC)) {
                eve_clipb_push(text, del_c);
                if (c == CH_CTRLC) return;
            }
        }

        text = widget->text + c1->ch;
        if (CHAR_VALID_INPUT(c)) {
            ins_c = utf8_enc(c, utf8_buf);
            ch_w = eve_font_ch_w(widget->font, c);
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
            ch_w = eve_font_str_w(widget->font, clipb);
        }
        if (widget->text_len + ins_c >= widget->text_size + del_c) {
            ins_c = 0;
            ch_w = 0;
        }
        if (ins_c != del_c) memmove(text + ins_c, text + del_c, widget->text_len - c2->ch + 1);
        if (ins_c) {
            if (c == CH_CTRLV) {
                memcpy(text, clipb, ins_c);
            } else if (ins_c > 1) {
                memcpy(text, utf8_buf, ins_c);
            } else {
                *text = utf8_buf[0];
            }
            c1->ch += ins_c;
        }
        if (c1 == cursor2) widget->cursor1 = widget->cursor2;
        if (cursor2->on) eve_textw_cursor_clear(widget, cursor2);
    }

    if ((ins_c == 0) && (del_c == 0)) return;

    widget->text_len += ins_c - del_c;
    for (i=cursor1->line; i<widget->line_len; i++) {
        widget->line[i] += ins_c - del_c;
    }

    r = cursor1->line;
    if (cursor1->line) r = eve_textw_text_update(widget, cursor1->line - 1, 1);
    if ((cursor1->line == 0) || (r == cursor1->line - 1)) r = eve_textw_text_update(widget, cursor1->line, 1);

    if (cursor1->line && (cursor1->ch < LINE_START(widget, cursor1->line))) {
        cursor1->line--;
        eve_textw_cursor_update(widget, cursor1);
        set_focus(widget, cursor1);
    } else if (cursor1->ch > LINE_END(widget, cursor1->line)) {
        while (cursor1->ch > LINE_END(widget, cursor1->line)) cursor1->line++;
        eve_textw_cursor_update(widget, cursor1);
        set_focus(widget, cursor1);
    } else {
        cursor1->x += ch_w;
    }
}

uint16_t eve_textw_text_update(EVETextWidget *widget, uint16_t line, int uievt) {
    int i;
    utf32_t ch;
    uint8_t ch_w;
    uint8_t ch_l;
    uint16_t word_w, line_w, line_b;
    uint16_t new_h;
    EVEWidget *_widget = &widget->w;
    EVEPage *page = _widget->page;

    word_w = 0;
    line_w = 0;
    line_b = LINE_EMPTY;

    i = LINE_START(widget, line);
    while (i < widget->text_size) {
        ch_l = utf8_dec(widget->text + i, &ch);
        if (!CHAR_VALID_INPUT(ch) && ch) {
            ch = 0;
            widget->text[i] = '\0';
            widget->text_len = i;
            widget->line[line] = LINE_EMPTY;
        }

        ch_w = eve_font_ch_w(widget->font, ch);
        if (ch <= 0x20) {
            if ((ch == '\n') || (ch == '\0')) {
                if (widget->line[line] == i) return line;
                widget->line[line] = i;
                line++;
                if ((ch == '\0') || (line == widget->line_size)) break;
                word_w = 0;
                line_w = 0;
                line_b = LINE_EMPTY;
            } else if (ch == ' ') {
                word_w = 0;
                line_w += ch_w;
                line_b = i;
            }
        } else {
            word_w += ch_w;
            line_w += ch_w;
        }
        if ((line_w > _widget->g.w) && (line_b != LINE_EMPTY)) {
            if (widget->line[line] == line_b) return line;
            widget->line[line] = line_b;
            line++;
            if (line == widget->line_size) {
                i = line_b;
                break;
            }
            line_w = word_w;
            line_b = LINE_EMPTY;
        }
        i += ch_l;
    }

    for (i=line; i<widget->line_size; i++) {
        widget->line[i] = LINE_EMPTY;
    }

    if (uievt && (widget->line_len != line)) {
        eve_view_uievt_push(&page->v, EVE_UIEVT_WIDGET_UPDATE_G, _widget);
    }
    widget->line_len = line;
    _widget->g.h = (widget->line_len + 1) * widget->font->h;

    return line;
}

void eve_textw_cursor_update(EVETextWidget *widget, EVETextCursor *cursor) {
    int i = LINE_START(widget, cursor->line);
    uint16_t x = 0;
    utf32_t ch;
    uint8_t ch_l;
    EVEWidget *_widget = &widget->w;

    while ((i < cursor->ch) && (i < LINE_END(widget, cursor->line))) {
        ch_l = utf8_dec(widget->text + i, &ch);
        x += eve_font_ch_w(widget->font, ch);
        i += ch_l;
    }
    cursor->x = x;
}

void eve_textw_cursor_set(EVETextWidget *widget, EVETextCursor *cursor, uint8_t tag, int16_t x) {
    int i;
    int16_t _x, _d;
    uint16_t c_line = LINE_EMPTY;
    utf32_t ch;
    uint8_t ch_w;
    uint8_t ch_l;
    EVEWidget *_widget = &widget->w;

    if ((tag >= _widget->tag0) && ((_widget->tagN == EVE_NOTAG) || (tag < _widget->tagN))) c_line = tag - _widget->tag0 + widget->line0;
    if (c_line < widget->line_len) {
        cursor->line = c_line;
    } else if (c_line == widget->line_len) {
        cursor->line = c_line - 1;
    } else if (!cursor->on) {
        return;
    }

    x -= _widget->g.x;

    _x = 0;
    _d = x;
    i = LINE_START(widget, cursor->line);
    while (i < LINE_END(widget, cursor->line)) {
        ch_l = utf8_dec(widget->text + i, &ch);
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

void eve_textw_cursor_clear(EVETextWidget *widget, EVETextCursor *cursor) {
    cursor->on = 0;
}
