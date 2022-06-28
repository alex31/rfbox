#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "airSensor.hpp"	
#include "oledDisplay.hpp"

int main (void)
{

  halInit();
  chSysInit();

#ifndef NOSHELL
  initHeap();
  consoleInit();
  consoleLaunch(); 
#endif
  
  airSensorStart();
  oledStart();

  while (true) {				
    palToggleLine(LINE_LED_GREEN);		
    chThdSleepMilliseconds(1000);		
  }
}


