#include <ch.h>
#include <hal.h>
#include <algorithm>

#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "rfm69.hpp"
#include "dip.hpp"
#include "modeTest.hpp"


Rfm69OokRadio radio(SPID1, LINE_RADIO_RESET);

namespace {
  constexpr uint32_t carrierFrequencyLow = 868'000'000;
  constexpr uint32_t carrierFrequencyHigh = 870'000'000;
  constexpr int8_t   ampLevelDbLow = -13;
  constexpr int8_t   ampLevelDbHigh = 18;
  constexpr uint32_t baudLow = 4800;
  constexpr uint32_t baudHigh = 19200;
  
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
  
  
  
  THD_WORKING_AREA(waBlinker, 304);
  void blinker (void *arg)		
  {
    (void)arg;				
    chRegSetThreadName("blinker");	
    
    while (true) {			
      palToggleLine(LINE_LED_GREEN);		
      chThdSleepMilliseconds(1000);	
    }
  }

}

int main (void)
{
  halInit();
  chSysInit();
  initHeap();		

  consoleInit();	
  consoleLaunch();
  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, &blinker, NULL);

  if (radio.init(spiCfg) != Rfm69Status::OK) {
    DebugTrace("radio.init failed");
  }
  const OpMode opMode = DIP::getDip(DIPSWITCH::RXTX) ? OpMode::RX : OpMode::TX;
  if (radio.setRfParam(opMode,
		       DIP::getDip(DIPSWITCH::FREQ) ? carrierFrequencyLow : carrierFrequencyHigh,
		       DIP::getDip(DIPSWITCH::PWRLVL) ? ampLevelDbLow : ampLevelDbHigh)
      != Rfm69Status::OK) {
    DebugTrace("radio.setRfParam failed");
  }
  // main thread does nothing
  DIP::start();
  if (DIP::getDip(DIPSWITCH::BER)) {
    ModeTest::start(opMode,
		    DIP::getDip(DIPSWITCH::BERBAUD) ? baudLow : baudHigh);
    }
  chThdSleep(TIME_INFINITE);
}


