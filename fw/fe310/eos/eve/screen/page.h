#include <stdint.h>

#define EVE_PAGE_SIZE_STACK     16

struct EVEPage;
struct EVEWidget;
struct EVEPageStack;

typedef void (*eve_page_constructor_t) (EVEWindow *window, struct EVEPageStack *);
typedef void (*eve_page_destructor_t) (struct EVEPage *);
typedef int (*eve_page_evt_handler_t) (struct EVEPage *, struct EVEWidget *, EVETouch *, uint16_t, uint8_t, int);
typedef void (*eve_page_g_updater_t) (struct EVEPage *, struct EVEWidget *);

typedef struct EVEPage {
    EVEView v;
    int16_t win_x;
    int16_t win_y;
    eve_page_evt_handler_t handle_evt;
    eve_page_g_updater_t update_g;
    eve_page_destructor_t destructor;
    struct EVEPageStack *stack;
    struct EVEWidget *widget_f;
} EVEPage;

typedef struct EVEPageStack {
    eve_page_constructor_t constructor[EVE_PAGE_SIZE_STACK];
    uint8_t level;
} EVEPageStack;

void eve_page_init(EVEPage *page, EVEWindow *window, EVEPageStack *stack, eve_view_touch_t touch, eve_view_draw_t draw, eve_page_evt_handler_t handle_evt, eve_page_g_updater_t update_g, eve_page_destructor_t destructor);
void eve_page_stack_init(EVEPageStack *stack);
void eve_page_create(EVEWindow *window, EVEPageStack *stack, eve_page_constructor_t constructor);
void eve_page_open(EVEPage *parent, eve_page_constructor_t constructor);
void eve_page_close(EVEPage *page);

int16_t eve_page_x(EVEPage *page, int16_t x);
int16_t eve_page_y(EVEPage *page, int16_t y);
int16_t eve_page_scr_x(EVEPage *page, int16_t x);
int16_t eve_page_scr_y(EVEPage *page, int16_t y);

void eve_page_set_focus(EVEPage *page, struct EVEWidget *widget, EVERect *focus);
struct EVEWidget *eve_page_get_focus(EVEPage *page);
int eve_page_rect_visible(EVEPage *page, EVERect *g);
