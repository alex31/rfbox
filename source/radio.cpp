#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"
#include "radio.hpp"
#include "stdutil.h"

namespace {
  const SPIConfig spiCfg = {
    .circular = false,
    .slave = false,
    .data_cb = NULL,
    .error_cb = NULL,
    /* HW dependent part.*/
    .ssline = LINE_RADIO_CS,
    /* 2.5 Mhz, 8 bits word, CPOL=0,  CPHA=0 */
    .cr1 = SPI_CR1_BR_2,
    .cr2 = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0 
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
