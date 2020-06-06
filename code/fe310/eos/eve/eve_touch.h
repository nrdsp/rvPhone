#include <stdint.h>

/* events */
#define EVE_TOUCH_ETYPE_TRACK           0x0001
#define EVE_TOUCH_ETYPE_TRACK_REG       0x0002
#define EVE_TOUCH_ETYPE_TRACK_START     0x0004
#define EVE_TOUCH_ETYPE_TRACK_STOP      0x0008
#define EVE_TOUCH_ETYPE_TAG             0x0010
#define EVE_TOUCH_ETYPE_TAG_UP          0x0020
#define EVE_TOUCH_ETYPE_POINT           0x0040
#define EVE_TOUCH_ETYPE_POINT_UP        0x0080
#define EVE_TOUCH_ETYPE_LPRESS          0x0100
#define EVE_TOUCH_ETYPE_TAP1            0x0200
#define EVE_TOUCH_ETYPE_TAP2            0x0400

#define EVE_TOUCH_EVT_MASK              0x0fff

#define EVE_TOUCH_ETYPE_TAG_MASK        (EVE_TOUCH_ETYPE_TAG        | EVE_TOUCH_ETYPE_TAG_UP)
#define EVE_TOUCH_ETYPE_TAP_MASK        (EVE_TOUCH_ETYPE_TAP1       | EVE_TOUCH_ETYPE_TAP2)
#define EVE_TOUCH_ETYPE_TRACK_MASK      (EVE_TOUCH_ETYPE_TRACK      | EVE_TOUCH_ETYPE_TRACK_START | EVE_TOUCH_ETYPE_TRACK_STOP)
#define EVE_TOUCH_ETYPE_POINT_MASK      (EVE_TOUCH_ETYPE_POINT      | EVE_TOUCH_ETYPE_POINT_UP)
#define EVE_TOUCH_ETYPE_TIMER_MASK      (EVE_TOUCH_OPT_LPRESS       | EVE_TOUCH_OPT_DTAP)

/* extended events */
#define EVE_TOUCH_ETYPE_TRACK_LEFT      0x1000
#define EVE_TOUCH_ETYPE_TRACK_RIGHT     0x2000
#define EVE_TOUCH_ETYPE_TRACK_UP        0x4000
#define EVE_TOUCH_ETYPE_TRACK_DOWN      0x8000

#define EVE_TOUCH_ETYPE_TRACK_X         (EVE_TOUCH_ETYPE_TRACK_LEFT | EVE_TOUCH_ETYPE_TRACK_RIGHT)
#define EVE_TOUCH_ETYPE_TRACK_Y         (EVE_TOUCH_ETYPE_TRACK_UP   | EVE_TOUCH_ETYPE_TRACK_DOWN)
#define EVE_TOUCH_ETYPE_TRACK_XY        (EVE_TOUCH_ETYPE_TRACK_X    | EVE_TOUCH_ETYPE_TRACK_Y)

/* tag options */
#define EVE_TOUCH_OPT_TRACK             EVE_TOUCH_ETYPE_TRACK
#define EVE_TOUCH_OPT_TRACK_REG         EVE_TOUCH_ETYPE_TRACK_REG
#define EVE_TOUCH_OPT_TRACK_X           0x04
#define EVE_TOUCH_OPT_TRACK_Y           0x08
#define EVE_TOUCH_OPT_TRACK_EXT         0x10
#define EVE_TOUCH_OPT_LPRESS            0x40
#define EVE_TOUCH_OPT_DTAP              0x80

#define EVE_TOUCH_OPT_TRACK_XY          (EVE_TOUCH_OPT_TRACK_X      | EVE_TOUCH_OPT_TRACK_Y)
#define EVE_TOUCH_OPT_TRACK_MASK        (EVE_TOUCH_OPT_TRACK        | EVE_TOUCH_OPT_TRACK_REG)
#define EVE_TOUCH_OPT_TIMER_MASK        (EVE_TOUCH_OPT_LPRESS       | EVE_TOUCH_OPT_DTAP)

typedef struct EVETouch {
    int x;
    int y;
    int vx;
    int vy;
    int x0;
    int y0;
    uint64_t t;
    uint16_t evt;
    uint8_t tag0;
    uint8_t tag;
    uint8_t tag_up;
    struct {
        uint8_t tag;
        uint8_t track;
        uint16_t val;
    } tracker;
} EVETouch;

typedef struct EVETouchTimer {
    uint8_t tag;
    uint8_t idx;
    uint16_t evt;
    int x0;
    int y0;
    void *p;
} EVETouchTimer;

typedef void (*eve_touch_handler_t) (void *, uint8_t, int);

void eve_init_touch(void);
void eve_handle_touch(void);
void eve_handle_time(void);

void eve_touch_set_handler(eve_touch_handler_t handler, void *handler_param);
EVETouch *eve_touch_evt(uint8_t tag0, int touch_idx, uint8_t tag_min, uint8_t tag_max, uint16_t *evt);
void eve_touch_set_opt(uint8_t tag, uint8_t opt);
uint8_t eve_touch_get_opt(uint8_t tag);
void eve_touch_clear_opt(void);
EVETouchTimer *eve_touch_get_timer(void);