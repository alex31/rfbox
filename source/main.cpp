#include <ch.h>
#include <hal.h>
#include <string>
#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "airSensor.hpp"	
#include "oledDisplay.hpp"

static THD_WORKING_AREA(waBlinker, 304);	
[[noreturn]] static void  blinker (void *arg)	
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
#ifndef NOSHELL
  consoleInit();
#endif
  
  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, &blinker, NULL);
  
#ifndef NOSHELL
  consoleLaunch(); 
#endif
  airSensorStart();
  oledStart();
  chThdSleep(TIME_INFINITE);
}


