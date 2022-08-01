#include <ch.h>
#include <hal.h>
#include <algorithm>

#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "dip.hpp"
#include "modeTest.hpp"
#include "radio.hpp"


namespace {
  constexpr uint32_t carrierFrequencyLow = 868'000'000;
  constexpr uint32_t carrierFrequencyHigh = 870'000'000;
  constexpr int8_t   ampLevelDbLow = 0;
  constexpr int8_t   ampLevelDbHigh = 18;
  constexpr uint32_t baudLow = 4800;
  constexpr uint32_t baudHigh = 19200;
  
}

int main (void)
{
  halInit();
  chSysInit();
  initHeap();		

  consoleInit();	
  consoleLaunch();

  RADIO::init();
  
  const OpMode opMode = DIP::getDip(DIPSWITCH::RXTX) ? OpMode::RX : OpMode::TX;
  if (opMode == OpMode::RX) 
    DebugTrace("mode RX");
  else
    DebugTrace("mode TX");

  if (DIP::getDip(DIPSWITCH::FREQ))
    DebugTrace("low frequency");
  else
    DebugTrace("high frequency");

  if (DIP::getDip(DIPSWITCH::PWRLVL))
     DebugTrace("low power");
  else
    DebugTrace("high power");
   
  
  if (RADIO::radio.setRfParam(opMode,
		       UARTMode::INVERTED,
		       DIP::getDip(DIPSWITCH::FREQ) ? carrierFrequencyLow : carrierFrequencyHigh,
		       DIP::getDip(DIPSWITCH::PWRLVL) ? ampLevelDbLow : ampLevelDbHigh)
      != Rfm69Status::OK) {
    DebugTrace("RADIO::radio.setRfParam failed");
  }
  // main thread does nothing
  DIP::start();
  if (DIP::getDip(DIPSWITCH::BER)) {
    DebugTrace("start BER mode");
    ModeTest::start(opMode,
		    DIP::getDip(DIPSWITCH::BERBAUD) ? baudLow : baudHigh);
    }
  chThdSleep(TIME_INFINITE);
}


