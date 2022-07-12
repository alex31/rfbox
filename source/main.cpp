#include <ch.h>
#include <hal.h>
#include <algorithm>

#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "notGate.hpp"
#include "rfm69.hpp"
#include "dip.hpp"



Rfm69OokRadio radio(SPID1);
constexpr uint32_t carrierFrequency = 868'000'000;
constexpr int8_t amplificationLevelDb = 18;

static const SPIConfig spiCfg = {
  .circular = false,
  .slave = false,
  .data_cb = NULL,
  .error_cb = NULL,
  /* HW dependent part.*/
  .ssline = LINE_RADIO_CS,
  /* 2.5 Mhz, 8 bits word, CPHA=1, CPOL=0 */
  .cr1 = SPI_CR1_CPHA |SPI_CR1_BR_2,
  .cr2 = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0 
};



static THD_WORKING_AREA(waBlinker, 304);
static void blinker (void *arg)		
{
  (void)arg;				
  chRegSetThreadName("blinker");	
  
  while (true) {			
    palToggleLine(LINE_LED_GREEN);		
    chThdSleepMilliseconds(1000);	
  }
}


int main (void)
{
  halInit();
  chSysInit();
  initHeap();		

  consoleInit();	
  consoleLaunch();
  notGateStart();
  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, &blinker, NULL);
  radio.init(spiCfg);
  radio.setRfParam(DIP::getDip0() == PAL_LOW ? OpModeMode::RX : OpModeMode::TX,
		   carrierFrequency,
		   amplificationLevelDb);
  // main thread does nothing
  chThdSleep(TIME_INFINITE);
}


