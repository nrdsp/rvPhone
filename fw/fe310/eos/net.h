#include <stdint.h>
#include "event.h"

/* common */
#define EOS_NET_MTU                 1500
#define EOS_NET_SIZE_BUF            EOS_NET_MTU

#define EOS_NET_MTYPE_SOCK          1
#define EOS_NET_MTYPE_RNG         	3
#define EOS_NET_MTYPE_POWER         4

#define EOS_NET_MTYPE_WIFI          5
#define EOS_NET_MTYPE_CELL          6
#define EOS_NET_MTYPE_SIP           7
#define EOS_NET_MTYPE_APP           8

#define EOS_NET_MAX_MTYPE           8

#define EOS_NET_MTYPE_FLAG_ONEW     0x40
#define EOS_NET_MTYPE_FLAG_REPL     0x80
#define EOS_NET_MTYPE_FLAG_MASK     0xc0

/* fe310 specific */
#define EOS_NET_SIZE_BUFQ           2

#define EOS_NET_FLAG_ONEW           0x1
#define EOS_NET_FLAG_SYNC           0x2
#define EOS_NET_FLAG_REPL           0x4

int eos_net_init(uint8_t wakeup_cause);
int eos_net_run(uint8_t wakeup_cause);
void eos_net_start(void);
void eos_net_stop(void);
int eos_net_sleep(uint32_t timeout);

void eos_net_bad_handler(unsigned char type, unsigned char *buffer, uint16_t len);
void eos_net_set_handler(unsigned char type, eos_evt_handler_t handler);
void eos_net_acquire_for_evt(unsigned char type, char acq);

void eos_net_acquire(void);
void eos_net_release(void);
unsigned char *eos_net_alloc(void);
void eos_net_free(unsigned char *buffer, unsigned char more);
int eos_net_xchg(unsigned char *type, unsigned char *buffer, uint16_t *len);
int eos_net_send(unsigned char type, unsigned char *buffer, uint16_t len);
int eos_net_send_async(unsigned char type, unsigned char *buffer, uint16_t len, unsigned char more);
int _eos_net_send(unsigned char type, unsigned char *buffer, uint16_t len, unsigned char async, unsigned char more);
