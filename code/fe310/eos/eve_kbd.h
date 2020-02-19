#include <stdint.h>

typedef void (*eve_kbd_input_handler_t) (void *, int);

typedef struct EVEKbd {
    uint32_t mem_addr;
    uint16_t mem_size;
    uint8_t key_count;
    uint8_t key_down;
    uint8_t key_modifier;
    uint8_t key_modifier_sticky;
    uint8_t key_modifier_lock;
    eve_kbd_input_handler_t putc;
} EVEKbd;

void eve_kbd_init(EVEKbd *kbd, eve_kbd_input_handler_t putc_f, uint32_t mem_addr, uint32_t *mem_next);
int eve_kbd_touch(EVEKbd *kbd, uint8_t tag0, int touch_idx, void *w);
uint8_t eve_kbd_draw(EVEKbd *kbd, char active);