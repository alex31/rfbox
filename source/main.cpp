#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "airSensor.hpp"	
#include "oledDisplay.hpp"
#include "windTestMode.hpp"

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
  windTestStart();
  oledStart();

  chThdSleep(TIME_INFINITE);
  while(true) {};
}


