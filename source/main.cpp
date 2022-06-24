#include <ch.h>
#include <hal.h>
#include <string>
#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "encoderTimer.hpp"	
#include "airSensor.hpp"	

volatile uint32_t ledBlinkPeriod = 1000;

static THD_WORKING_AREA(waBlinker, 304);	
[[noreturn]] static void  blinker (void *arg)	
{
  (void)arg;					
  chRegSetThreadName("blinker");		

  while (true) {				
    palToggleLine(LINE_LED_GREEN);		
    chThdSleepMilliseconds(ledBlinkPeriod);		
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
  chThdSleep(TIME_INFINITE);
}


