#include <stdlib.h>
#include <string.h>

#include "eve.h"
#include "text.h"

#define CH_BS               0x08
#define CH_DEL              0x7f
#define CH_EOF              0x1a

#define TEXTW_TMODE_CURSOR  1
#define TEXTW_TMODE_SCROLL  2

#define LINE_LEN(w,l)       ((l) ? (w->line[l] ? w->line[l] - w->line[(l) - 1] - 1 : 0) : w->line[l])
#define LINE_START(w,l)     ((l) ? w->line[(l) - 1] + 1 : 0)
#define LINE_END(w,l)       (w->line[l])
#define LINE_EMPTY          0xffff

void eve_textw_init(EVETextWidget *widget, uint16_t x, uint16_t y, uint16_t w, uint16_t h, char *text, uint64_t text_size, uint16_t *line, uint16_t line_size, EVEFont *font) {
    memset(widget, 0, sizeof(EVETextWidget));
    widget->w.type = EVE_WIDGET_TYPE_TEXT;
    widget->w.x = x;
    widget->w.y = y;
    widget->w.w = w;
    widget->w.h = h;
    widget->w.touch = eve_textw_touch;
    widget->w.draw = eve_textw_draw;
    widget->w.putc = eve_textw_putc;
    widget->text = text;
    widget->text_size = text_size;
    widget->line = line;
    widget->line_size = line_size;
    widget->font = font;
    if (text_size && line_size) eve_textw_update(widget, 0);
}

static EVETextCursor *cursor_prox(EVETouch *t, EVETextWidget *widget, EVETextCursor *cursor, short *dx, short *dl) {
    EVEWidget *_widget = (EVEWidget *)widget;
    int x = (int)t->x0 - _widget->x;
    int l = (int)t->tag0 - widget->tag0 + widget->line0;
    int _dx, _dl;

    *dx = cursor->x - x;
    *dl = cursor->line - l;

    _dx = *dx < 0 ? -(*dx) : *dx;
    _dl = *dl < 0 ? -(*dl) : *dl;
    if ((_dx <= widget->font->h) && (_dl <= 1)) return cursor;
    return NULL;
}

int eve_textw_touch(EVEWidget *_widget, EVEScreen *screen, uint8_t tag0, int touch_idx) {
    EVETextWidget *widget = (EVETextWidget *)_widget;
    EVETouch *t;
    uint16_t evt;

    if (touch_idx > 0) return 0;

    t = eve_touch_evt(tag0, touch_idx, widget->tag0, widget->tagN, &evt);
    if (t && evt) {
        EVETextCursor *t_cursor = NULL;
        EVETextCursor *a_cursor = NULL;
        short dx, dl;

        if (!(evt & EVE_TOUCH_ETYPE_TRACK) || !widget->track.mode) {
            if (widget->cursor2.on && widget->tag0) {
                t_cursor = cursor_prox(t, widget, &widget->cursor2, &dx, &dl);
            }
            if (widget->cursor1.on && widget->tag0 && (t_cursor == NULL)) {
                t_cursor = cursor_prox(t, widget, &widget->cursor1, &dx, &dl);
            }
        }
        if (evt & EVE_TOUCH_ETYPE_TRACK) {
            if (!widget->track.mode) {
                widget->track.cursor = t_cursor;
                if (t_cursor) {
                    widget->track.mode = TEXTW_TMODE_CURSOR;
                    widget->track.dx = dx;
                    widget->track.dl = dl;
                } else {
                    widget->track.mode = TEXTW_TMODE_SCROLL;
                }
            }

            if (widget->track.mode == TEXTW_TMODE_CURSOR) {
                eve_textw_cursor_set(widget, widget->track.cursor, t->tag + widget->track.dl, t->x + widget->track.dx);
            } else {
                screen->handle_evt(screen, _widget, tag0, touch_idx, t, evt);
                if (evt & EVE_TOUCH_ETYPE_TRACK_Y) {
                    // do scroll
                } else {
                    // go back / forward
                }
            }
        } else if (evt & EVE_TOUCH_ETYPE_LPRESS) {
            if (!t_cursor) {
                if (widget->cursor2.on) {
                    // show context w cut / copy / paste
                } else if (widget->cursor1.on) {
                    // show context w paste / select
                    eve_textw_cursor_set(widget, &widget->cursor2, t->tag, t->x);
                } else {
                    // show context w paste
                }
            }
        } else if (evt & EVE_TOUCH_ETYPE_TAG_UP) {
            if (!t_cursor) {
                if (widget->cursor2.on) {
                    a_cursor = &widget->cursor2;
                    eve_textw_cursor_set(widget, a_cursor, t->tag_up, t->x);
                } else {
                    a_cursor = &widget->cursor1;
                    eve_textw_cursor_set(widget, a_cursor, t->tag_up, t->x);
                }
            }
        }
        if (evt & EVE_TOUCH_ETYPE_POINT_UP) {
            widget->track.mode = 0;
            widget->track.cursor = NULL;
            widget->track.dx = 0;
            widget->track.dl = 0;
        }

        if (a_cursor) {
            EVERect a;

            a.x1 = _widget->x;
            a.y1 = _widget->y + a_cursor->line * widget->font->h;
            a.x2 = _widget->x + _widget->w;
            a.y2 = _widget->y + (a_cursor->line + 2) * widget->font->h;
            screen->move(screen, _widget, &a);
        }
        if (widget->cursor1.on) return 1;
    }
    return 0;
}

static void _draw_line(EVETextWidget *widget, uint16_t l, uint16_t ch, uint16_t len, uint16_t x1, uint16_t x2, char s) {
    EVEWidget *_widget = (EVEWidget *)widget;

    if (x1 != x2) {
        eve_cmd_dl(TAG(widget->tagN));
        eve_cmd_dl(BEGIN(EVE_RECTS));
        if (!s) eve_cmd_dl(COLOR_MASK(0 ,0 ,0 ,0));
        eve_cmd_dl(VERTEX2F(_widget->x + x1, _widget->y + l * widget->font->h));
        eve_cmd_dl(VERTEX2F(_widget->x + x2, _widget->y + (l + 1) * widget->font->h));
        if (!s) eve_cmd_dl(COLOR_MASK(1 ,1 ,1 ,1));
        if (len) {
            if (s) eve_cmd_dl(COLOR_RGB(0, 0, 0));
            eve_cmd(CMD_TEXT, "hhhhpb", _widget->x, _widget->y + l * widget->font->h, widget->font->idx, 0, widget->text + ch, len, 0);
            if (s) eve_cmd_dl(COLOR_RGB(255, 255, 255));
        }
        eve_touch_set_opt(widget->tagN, EVE_TOUCH_OPT_TRACK | EVE_TOUCH_OPT_TRACK_XY | EVE_TOUCH_OPT_INERT);
    }
}

static void _draw_cursor(EVETextWidget *widget, EVETextCursor *cursor) {
    uint16_t x, y;
    EVEWidget *_widget = (EVEWidget *)widget;

    x = _widget->x + cursor->x;
    y = _widget->y + cursor->line * widget->font->h;
    eve_cmd_dl(BEGIN(EVE_LINES));
    eve_cmd_dl(VERTEX2F(x, y));
    eve_cmd_dl(VERTEX2F(x, y + widget->font->h));
    eve_cmd_dl(END());
}

uint8_t eve_textw_draw(EVEWidget *_widget, EVEScreen *screen, uint8_t tag0, char active) {
    int i;
    int line0, lineN;
    int _line0, _lineN;
    char s, w, lineNvisible;
    char ro = screen->ro;
    EVERect *r = screen->visible;
    EVETextWidget *widget = (EVETextWidget *)_widget;
    EVETextCursor *c1, *c2;

    if (ro) {
        widget->line0 = 0;
        widget->tag0 = 0;
        widget->tagN = 0;
        for (i=0; i<widget->line_len; i++) {
            eve_cmd(CMD_TEXT, "hhhhpb", _widget->x, _widget->y + i * widget->font->h, widget->font->idx, 0, widget->text + LINE_START(widget, i), LINE_LEN(widget, i), 0);
        }
        return 0;
    }

    if (widget->cursor1.on && !active) {
        eve_textw_cursor_clear(&widget->cursor1);
        eve_textw_cursor_clear(&widget->cursor2);
    }

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

    _line0 = line0 = ((int)r->y1 - _widget->y) / widget->font->h;
    _lineN = lineN = ((int)r->y2 - _widget->y) / widget->font->h + 1;
    // y1 = (int)y_min - y_offset
    // y2 = (int)y_max - y_offset
    // line0 = ((int)y_min - _widget->y - y_offset) / widget->font->h;
    // lineN = ((int)y_max - _widget->y - y_offset) / widget->font->h + 1;
    if (line0 < 0) line0 = 0;
    if (lineN < 0) lineN = 0;
    if (line0 > widget->line_len) line0 = widget->line_len;
    if (lineN > widget->line_len) lineN = widget->line_len;
    lineNvisible = (lineN >= _line0) && (lineN < _lineN);
    widget->line0 = line0;
    widget->tag0 = tag0;
    widget->tagN = tag0;

    s = 0;
    w = lineNvisible || (line0 < lineN);
    if (w) {
        eve_cmd_dl(SAVE_CONTEXT());
        eve_cmd_dl(VERTEX_FORMAT(0));
        eve_cmd_dl(LINE_WIDTH(1));
    }
    for (i=line0; i<lineN; i++) {
        if (c1 && !s && (c1->line == i)) {
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
            _draw_line(widget, i, LINE_START(widget, i), l1, 0, c1->x, 0);
            _draw_line(widget, i, c1->ch, l2, c1->x, s ? _widget->w : c2->x, 1);
            if (!s) {
                _draw_line(widget, i, c2->ch, l3, c2->x, _widget->w, 0);
                c1 = NULL;
                c2 = NULL;
            }
        } else if (s && (c2->line == i)) {
            int l1 = c2->ch - LINE_START(widget, i);
            int l2 = LINE_START(widget, i) + LINE_LEN(widget, i) - c2->ch;

            _draw_line(widget, i, LINE_START(widget, i), l1, 0, c2->x, 1);
            _draw_line(widget, i, c2->ch, l2, c2->x, _widget->w, 0);
            c1 = NULL;
            c2 = NULL;
            s = 0;
        } else {
            _draw_line(widget, i, LINE_START(widget, i), LINE_LEN(widget, i), 0, _widget->w, s);
        }
        if (widget->tagN && (widget->tagN < 0xfe)) widget->tagN++;
    }
    if (w) {
        if (widget->cursor1.on && (widget->cursor1.line >= line0) && (widget->cursor1.line < lineN)) _draw_cursor(widget, &widget->cursor1);
        if (widget->cursor2.on && (widget->cursor2.line >= line0) && (widget->cursor2.line < lineN)) _draw_cursor(widget, &widget->cursor2);
        if (lineNvisible) {
            _draw_line(widget, lineN, 0, 0, 0, _widget->w, 0);
        } else {
            widget->tagN--;
        }
        eve_cmd_dl(RESTORE_CONTEXT());
    } else {
        widget->line0 = 0;
        widget->tag0 = 0;
        widget->tagN = 0;
    }
    return widget->tagN;
}

void eve_textw_putc(void *_w, int c) {
    int i, r;
    EVETextWidget *widget = (EVETextWidget *)_w;
    EVETextCursor *cursor1 = &widget->cursor1;
    EVETextCursor *cursor2 = &widget->cursor2;

    if (c == CH_EOF) {
        eve_textw_cursor_clear(&widget->cursor1);
        eve_textw_cursor_clear(&widget->cursor2);
        return;
    }

    if (cursor1->on) {
        char m = 0;
        char *text = widget->text + cursor1->ch;

        switch (c) {
            case CH_BS:
                if (cursor1->ch > 0) {
                    cursor1->x -= widget->font->w[*(text - 1)];
                    memmove(text - 1, text, widget->text_len - cursor1->ch + 1);
                    widget->text_len--;
                    cursor1->ch--;
                    m = -1;
                }
                break;
            case CH_DEL:
                if (cursor1->ch < widget->text_len) {
                    memmove(text, text + 1, widget->text_len - cursor1->ch);
                    widget->text_len--;
                    m = -1;
                }
                break;
            default:
                if (widget->text_len < (widget->text_size - 1)) {
                    cursor1->x += widget->font->w[c];
                    memmove(text + 1, text, widget->text_len - cursor1->ch + 1);
                    *text = c;
                    widget->text_len++;
                    cursor1->ch++;
                    m = 1;
                }
                break;
        }

        if (m == 0) return;
        for (i=cursor1->line; i<widget->line_len; i++) {
            widget->line[i] += m;
        }
        r = cursor1->line;
        if (cursor1->line) r = eve_textw_update(widget, cursor1->line - 1);
        if (r == cursor1->line) r = eve_textw_update(widget, cursor1->line);
        if (r < 0) return;

        if (cursor1->ch > widget->text_len) cursor1->ch = widget->text_len;
        if (r != cursor1->line) {
            if (cursor1->line && (cursor1->ch < LINE_START(widget, cursor1->line))) {
                cursor1->line--;
                eve_textw_cursor_update(widget, cursor1);
            } else if (cursor1->ch > LINE_END(widget, cursor1->line)) {
                cursor1->line++;
                eve_textw_cursor_update(widget, cursor1);
            }
        }
    }
}

int eve_textw_update(EVETextWidget *widget, uint16_t line) {
    int i;
    char ch;
    uint8_t ch_w;
    uint16_t word_w, line_w, line_b;
    EVEWidget *_widget = (EVEWidget *)widget;

    word_w = 0;
    line_w = 0;
    line_b = LINE_EMPTY;
    for (i=LINE_START(widget, line); i<widget->text_size; i++) {
        ch = widget->text[i];
        ch_w = widget->font->w[ch];
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
            } else {
                return EVE_ERR_TEXT;
            }
        } else if (ch < 0x7f) {
            word_w += ch_w;
            line_w += ch_w;
        } else {
            return EVE_ERR_TEXT;
        }
        if ((line_w > _widget->w) && (line_b != LINE_EMPTY)) {
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
    }

    if (i == widget->text_size) i--;
    widget->text[i] = '\0';
    widget->text_len = i;

    widget->line_len = line;
    _widget->h = (line + 1) * widget->font->h;
    for (i=line; i<widget->line_size; i++) {
        widget->line[i] = LINE_EMPTY;
    }
}

void eve_textw_cursor_update(EVETextWidget *widget, EVETextCursor *cursor) {
    int i;
    uint16_t x;

    x = 0;
    for (i=LINE_START(widget, cursor->line); i<LINE_END(widget, cursor->line); i++) {
        if (cursor->ch == i) break;
        x += widget->font->w[widget->text[i]];
    }
    cursor->x = x;
}

void eve_textw_cursor_set(EVETextWidget *widget, EVETextCursor *cursor, uint8_t tag, uint16_t x) {
    int i;
    uint16_t _x, _d;
    uint16_t c_line = LINE_EMPTY;
    EVEWidget *_widget = (EVEWidget *)widget;

    if (widget->tag0 == 0) return;
    if ((tag >= widget->tag0 && tag <= widget->tagN)) c_line = tag - widget->tag0 + widget->line0;
    if (c_line < widget->line_len) {
        cursor->line = c_line;
    } else if (c_line == widget->line_len) {
        cursor->line = c_line - 1;
    } else if (!cursor->on) {
        return;
    }

    x -= _widget->x;
    _x = 0;
    _d = x;
    for (i=LINE_START(widget, cursor->line); i<LINE_END(widget, cursor->line); i++) {
        _x += widget->font->w[widget->text[i]];
        if (_x >= x) {
            if (_x - x < _d) {
                i++;
            } else {
                _x -= widget->font->w[widget->text[i]];
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

void eve_textw_cursor_clear(EVETextCursor *cursor) {
    cursor->on = 0;
}