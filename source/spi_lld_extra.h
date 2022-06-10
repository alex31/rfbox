#pragma once

#include <ch.h>
#include <hal.h>


#ifdef __cplusplus
extern "C" {
#endif

  void spi_lld_polled_send(SPIDriver *spip, uint16_t frame);
  uint16_t spi_lld_polled_receive(SPIDriver *spip);
  void spi_lld_wait_until_not_busy(SPIDriver *spip);
#ifdef __cplusplus
}
#endif

