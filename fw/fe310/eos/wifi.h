#include <stdint.h>
#include "event.h"

#define EOS_WIFI_MTYPE_SCAN         1
#define EOS_WIFI_MTYPE_AUTH         2
#define EOS_WIFI_MTYPE_CONNECT      3
#define EOS_WIFI_MTYPE_DISCONNECT   4

#define EOS_WIFI_MAX_MTYPE          5

void eos_wifi_netinit(void);
void eos_wifi_set_handler(unsigned char mtype, eos_evt_handler_t handler);
eos_evt_handler_t eos_wifi_get_handler(unsigned char mtype);

int eos_wifi_scan(unsigned char *buffer);
int eos_wifi_auth(const char *ssid, const char *pass, unsigned char *buffer);
int eos_wifi_connect(unsigned char *buffer);
int eos_wifi_disconnect(unsigned char *buffer);
