#include <stdlib.h>
#include <stdio.h>

#include <esp_log.h>

#include "eos.h"
#include "at_cmd.h"
#include "cell.h"

static char cmd[256];
static int cmd_len;

void eos_cell_ussd_handler(unsigned char mtype, unsigned char *buffer, uint16_t size) {
    int rv;

    buffer += 1;
    size -= 1;
    switch (mtype) {
        case EOS_CELL_MTYPE_USSD_REQUEST:
            if (size == 0) return;

            buffer[size] = '\0';
            cmd_len = snprintf(cmd, sizeof(cmd), "AT+CUSD=1,\"%s\",15\r", buffer);
            if ((cmd_len < 0) || (cmd_len >= sizeof(cmd))) return;

            rv = eos_modem_take(1000);
            if (rv) return;

            at_cmd(cmd);
            rv = at_expect("^OK", "^ERROR", 1000);

            eos_modem_give();

            break;
    }

}

void eos_cell_ussd_init(void) {}