#include <stdlib.h>
#include <stdio.h>

#include <esp_log.h>

#include "eos.h"
#include "cell.h"


void eos_cell_sms_handler(unsigned char mtype, unsigned char *buffer, uint16_t size) {
    int rv;

    rv = eos_modem_take(1000);
    if (rv) return;

    buffer += 1;
    size -= 1;
    switch (mtype) {
    }

    eos_modem_give();
}

void eos_cell_sms_init(void) {}
