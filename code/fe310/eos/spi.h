#include <stdint.h>

#include "encoding.h"
#include "platform.h"

#include "spi_def.h"

#define SPI_IOF_MASK            (((uint32_t)1 << IOF_SPI1_SCK) | ((uint32_t)1 << IOF_SPI1_MOSI) | ((uint32_t)1 << IOF_SPI1_MISO))
