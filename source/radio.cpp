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
    .cr2 = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
  };
}

namespace Radio {
  Rfm69BaseRadio *radio = nullptr;

  void init()
  {
    Radio::radio = new Rfm69OokRadio(SPID1, LINE_RADIO_RESET);
    if (Radio::radio->init(spiCfg) != Rfm69Status::OK) {
      DebugTrace("radio->init failed");
    }
  }
  
}
