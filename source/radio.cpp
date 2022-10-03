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
    /* frequency defined in hardwareConf.hpp, CPOL=0,  CPHA=0 */
    .cr1 = spiBr12Baud,
     /* 8 bits word */
   .cr2 = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
  };
}

namespace Radio {
  Rfm69BaseRadio *radio = nullptr;

  void init(Ope::Mode opMode)
  {
    if ((opMode == Ope::Mode::RF_RX_EXTERNAL_OOK) or
	(opMode == Ope::Mode::RF_TX_EXTERNAL_OOK))
      Radio::radio = new Rfm69OokRadio(SPID1, LINE_RADIO_RESET);
    else if ((opMode == Ope::Mode::RF_RX_EXTERNAL_FSK) or
	     (opMode == Ope::Mode::RF_TX_EXTERNAL_FSK))
      Radio::radio = new Rfm69FskRadio(SPID1, LINE_RADIO_RESET);
    else
      chSysHalt("internal logic error : opMode not handled");
    
    if (Radio::radio->init(spiCfg) != Rfm69Status::OK) {
      DebugTrace("radio->init failed");
    }
  }
  
}
