#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "encoderTimer.hpp"	


volatile uint32_t ledBlinkPeriod = 1000;
EncoderModeLPTimer1 lptim1d;


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

static THD_WORKING_AREA(waLptim1, 304);	
[[noreturn]] static void  lptim1 (void *arg)	
{
  (void)arg;					
  chRegSetThreadName("lptim1");		
  lptim1d.start();
  while (true) {				
    chThdSleepMilliseconds(200);
    auto [u, cnt] = lptim1d.getCnt();
    if (u) {
      DebugTrace("lptim1 CNT = %u", LPTIM1->CNT);
    }
  }
}



int main (void)
{

  halInit();
  chSysInit();
  initHeap();	
  consoleInit();
  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, &blinker, NULL);
  chThdCreateStatic(waLptim1, sizeof(waLptim1), NORMALPRIO, &lptim1, NULL);
  
  consoleLaunch(); 

  chThdSleep(TIME_INFINITE);
}


