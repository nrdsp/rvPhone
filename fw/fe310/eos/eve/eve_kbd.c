#include <stdlib.h>
#include <string.h>

#include "eve.h"
#include "eve_kbd.h"

#define KEY_SPACERX     3
#define KEY_SPACERY     5
#define KEY_FONT        29
#define MOD_FONT        21

#define KEY_BS          0x08
#define KEY_RET         0x0a

#define FLAG_SHIFT      0x01
#define FLAG_CTRL       0x02
#define FLAG_FN         0x04

#define TAG_SHIFT       0x11
#define TAG_CTRL        0x12
#define TAG_FN          0x13

void eve_kbd_init(EVEKbd *kbd, EVERect *g, uint32_t mem_addr, uint32_t *mem_next) {
    uint16_t mem_size;

    kbd->g = *g;
    kbd->mem_addr = mem_addr;
    kbd->key_modifier = 0;
    kbd->key_modifier_sticky = 0;
    kbd->key_modifier_lock = 0;
    kbd->key_count = 0;
    kbd->key_down = 0;
    kbd->putc = NULL;
    kbd->param = NULL;

    kbd->key_down = 0xff;
    eve_write16(REG_CMD_DL, 0);
    eve_kbd_draw(kbd);
    eve_cmd_exec(1);
    mem_size = eve_read16(REG_CMD_DL);
    eve_cmd(CMD_MEMCPY, "www", mem_addr, EVE_RAM_DL, mem_size);
    eve_cmd_exec(1);
    kbd->key_down = 0;
    kbd->mem_size = mem_size;

    *mem_next = kbd->mem_addr + kbd->mem_size;
}

void eve_kbd_close(EVEKbd *kbd) {
    kbd->key_modifier = 0;
    kbd->key_modifier_sticky = 0;
    kbd->key_modifier_lock = 0;
    kbd->key_count = 0;
    kbd->key_down = 0;
    kbd->putc = NULL;
    kbd->param = NULL;
}

void eve_kbd_set_handler(EVEKbd *kbd, eve_kbd_input_handler_t putc, void *param) {
    kbd->putc = putc;
    kbd->param = param;
}

int eve_kbd_touch(EVEKbd *kbd, EVETouch *touch, uint16_t evt, uint8_t tag0) {
    int ret;

    evt = eve_touch_evt(touch, evt, tag0, 1, 126);
    if (touch && evt) {
        int8_t touch_idx = eve_touch_get_idx(touch);

        if (evt & EVE_TOUCH_ETYPE_TAG) {
            uint8_t _tag = touch->tag;

            if (_tag >= TAG_SHIFT && _tag <= TAG_FN) {
                if (touch_idx == 0) {
                    uint8_t f = (1 << (_tag - TAG_SHIFT));

                    kbd->key_modifier = f;
                    kbd->key_modifier_sticky &= f;
                    kbd->key_modifier_lock &= f;
                    if (kbd->key_modifier_lock & f) {
                        kbd->key_modifier_lock &= ~f;
                    } else if (kbd->key_modifier_sticky & f) {
                        kbd->key_modifier_sticky &= ~f;
                        kbd->key_modifier_lock |= f;
                    } else {
                        kbd->key_modifier_sticky |= f;
                    }
                }
            } else {
                kbd->key_count++;
                kbd->key_down = _tag;
                if (kbd->putc) {
                    int c = _tag;

                    if ((kbd->key_modifier & FLAG_CTRL) && (_tag >= '?') && (_tag <= '_')) c = (_tag - '@') & 0x7f;
                    kbd->putc(kbd->param, c);
                }
            }
        }
        if (evt & EVE_TOUCH_ETYPE_TAG_UP) {
            uint8_t _tag = touch->tag_up;

            if (_tag >= TAG_SHIFT && _tag <= TAG_FN) {
                if (touch_idx == 0) {
                    uint8_t f = (1 << (_tag - TAG_SHIFT));

                    if (!((kbd->key_modifier_lock | kbd->key_modifier_sticky) & f)) {
                        kbd->key_modifier &= ~f;
                    }
                }
            } else {
                if (kbd->key_count) kbd->key_count--;
                if (!kbd->key_count) kbd->key_down = 0;
                if (kbd->key_modifier_sticky) {
                    if (touch_idx == 0) kbd->key_modifier &= ~kbd->key_modifier_sticky;
                    kbd->key_modifier_sticky = 0;
                }
            }
        }
        ret = 1;
    } else {
        ret = 0;
    }

    return ret;
}

uint8_t eve_kbd_draw(EVEKbd *kbd) {
    if (kbd->key_down || kbd->key_modifier) {
        int x = kbd->g.x;
        int y = kbd->g.y;
        int w = kbd->g.w;
        int row_h = kbd->g.h / 5;
        int key_w = (w - 9 * KEY_SPACERX) / 10 + 1;
        int mod_w = key_w + key_w / 2;
        int key_h = row_h - KEY_SPACERY;

        eve_cmd_dl(SAVE_CONTEXT());
        eve_cmd(CMD_KEYS, "hhhhhhs", x, y + row_h * 0, w, key_h, KEY_FONT, kbd->key_down, kbd->key_modifier & (FLAG_FN | FLAG_SHIFT) ? "!@#$%^&*()" : (kbd->key_modifier & FLAG_CTRL ? " @[\\]^_?  " : "1234567890"));
        eve_cmd(CMD_KEYS, "hhhhhhs", x, y + row_h * 1, w, key_h, KEY_FONT, kbd->key_down, kbd->key_modifier & FLAG_FN ? "-_=+[]{}\\|" : kbd->key_modifier & (FLAG_SHIFT | FLAG_CTRL) ? "QWERTYUIOP" : "qwertyuiop");
        eve_cmd(CMD_KEYS, "hhhhhhs", x + key_w / 2, y + row_h * 2, w - key_w, key_h, KEY_FONT, kbd->key_down, kbd->key_modifier & FLAG_FN ? "`~   ;:'\"" : kbd->key_modifier & (FLAG_SHIFT | FLAG_CTRL) ? "ASDFGHJKL" : "asdfghjkl");
        eve_cmd(CMD_KEYS, "hhhhhhs", x + mod_w + KEY_SPACERX, y + row_h * 3, w - 2 * (mod_w + KEY_SPACERX), key_h, KEY_FONT, kbd->key_down, kbd->key_modifier & FLAG_FN ? " ,.<>/?" : kbd->key_modifier & (FLAG_SHIFT | FLAG_CTRL) ? "ZXCVBNM" : "zxcvbnm");
        eve_cmd_dl(TAG(TAG_SHIFT));
        eve_cmd(CMD_BUTTON, "hhhhhhs", x, y + row_h * 3, mod_w, key_h, MOD_FONT, kbd->key_modifier & FLAG_SHIFT ? EVE_OPT_FLAT : 0, "shift");
        eve_cmd_dl(TAG(KEY_BS));
        eve_cmd(CMD_BUTTON, "hhhhhhs", x + w - mod_w, y + row_h * 3, mod_w, key_h, MOD_FONT, kbd->key_down == KEY_BS ? EVE_OPT_FLAT : 0, "del");
        eve_cmd_dl(TAG(TAG_FN));
        eve_cmd(CMD_BUTTON, "hhhhhhs", x, y + row_h * 4, mod_w, key_h, MOD_FONT, kbd->key_modifier & FLAG_FN ? EVE_OPT_FLAT : 0, "fn");
        eve_cmd_dl(TAG(TAG_CTRL));
        eve_cmd(CMD_BUTTON, "hhhhhhs", x + mod_w + KEY_SPACERX, y + row_h * 4, mod_w, key_h, MOD_FONT, kbd->key_modifier & FLAG_CTRL ? EVE_OPT_FLAT : 0, "ctrl");
        eve_cmd_dl(TAG(' '));
        eve_cmd(CMD_BUTTON, "hhhhhhs", x + 2 * (mod_w + KEY_SPACERX), y + row_h * 4, w - 3 * (mod_w + KEY_SPACERX), key_h, MOD_FONT, kbd->key_down == ' ' ? EVE_OPT_FLAT : 0, "");
        eve_cmd_dl(TAG(KEY_RET));
        eve_cmd(CMD_BUTTON, "hhhhhhs", x + w - mod_w, y + row_h * 4, mod_w, key_h, MOD_FONT, kbd->key_down == KEY_RET ? EVE_OPT_FLAT : 0, "ret");
        eve_cmd_dl(RESTORE_CONTEXT());
    } else {
        eve_cmd(CMD_APPEND, "ww", kbd->mem_addr, kbd->mem_size);
    }

    return 0x80;
}
