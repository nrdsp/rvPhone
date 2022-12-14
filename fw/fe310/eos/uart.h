#include <stdint.h>

#define EOS_UART_ETYPE_TX       1
#define EOS_UART_ETYPE_RX       2

#define EOS_UART_MAX_ETYPE      2

#define EOS_UART_SPEED          115200

typedef void (*eos_uart_handler_t) (unsigned char);

int eos_uart_init(uint8_t wakeup_cause);
void eos_uart_start(void);
void eos_uart_stop(void);
void eos_uart_speed(uint32_t baud_rate);

void eos_uart_set_handler(unsigned char type, eos_uart_handler_t handler);

void eos_uart_txwm_set(uint8_t wm);
void eos_uart_txwm_clear(void);
void eos_uart_rxwm_set(uint8_t wm);
void eos_uart_rxwm_clear(void);
int eos_uart_putc(int c, char b);
int eos_uart_getc(char b);