#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"
#include "radio.hpp"
#include "stdutil.h"
#include "hardwareConf.hpp"

namespace {
  const SPIConfig spiCfg = {
    .circular = false,
    .slave = false,
    .data_cb = NULL,
    .error_cb = [](SPIDriver *) {chSysHalt("SPI error_cb");},
    /* HW dependent part.*/
    .ssline = LINE_RADIO_CS,
    /* 2.5 Mhz, 8 bits word, CPOL=0,  CPHA=0 */
    .cr1 = spiBr12Baud,
#ifdef STM32L432_MCUCONF
    .cr2 = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
#elif defined STM32F405_MCUCONF
    .cr2 = 0
#else
    #error "Unknown target"
#endif
  };
}

namespace RADIO {
  Rfm69OokRadio radio(SPID1, LINE_RADIO_RESET);

  void init()
  {
    if (RADIO::radio.init(spiCfg) != Rfm69Status::OK) {
      DebugTrace("radio.init failed");
    }
  }
  
}
