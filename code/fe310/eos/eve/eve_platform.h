#include <stdint.h>

#include "../spi.h"
#include "../timer.h"

#define EVE_ETYPE_INTR      1
#define EVE_PIN_INTR        0

#define EVE_RTC_FREQ        EOS_TIMER_RTC_FREQ

#define EVE_SPI_FLAG_BSWAP  EOS_SPI_FLAG_BSWAP
#define EVE_SPI_FLAG_TX     EOS_SPI_FLAG_TX

#define eve_spi_cs_set      eos_spi_cs_set
#define eve_spi_cs_clear    eos_spi_cs_clear
#define eve_spi_flush       eos_spi_flush
#define eve_spi_xchg8       eos_spi_xchg8
#define eve_spi_xchg16      eos_spi_xchg16
#define eve_spi_xchg24      eos_spi_xchg24
#define eve_spi_xchg32      eos_spi_xchg32

void eve_time_sleep(uint32_t ms);
void eve_timer_set(uint32_t ms);
void eve_timer_clear(void);
uint64_t eve_time_get_tick(void);

void eve_init_platform(void);