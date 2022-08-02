#include <ch.h>
#include <hal.h>
#include <algorithm>

#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "dip.hpp"
#include "modeTest.hpp"
#include "radio.hpp"
#include "operations.hpp"
#include "hardwareConf.hpp"

int main (void)
{
  halInit();
  chSysInit();
  initHeap();		

  consoleInit();	
  consoleLaunch();

  RADIO::init();
  
  const RfMode rfMode = DIP::getDip(DIPSWITCH::RXTX) ? RfMode::RX : RfMode::TX;
  if (rfMode == RfMode::RX) 
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
   
  

  // main thread does nothing
  DIP::start();
  if (DIP::getDip(DIPSWITCH::BER)) {
    DebugTrace("start BER mode");
    ModeTest::start(rfMode,
		    DIP::getDip(DIPSWITCH::BERBAUD) ? baudLow : baudHigh);
    }
  chThdSleep(TIME_INFINITE);
}


