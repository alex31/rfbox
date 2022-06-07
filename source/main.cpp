#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "tiedGpios.hpp"
#include "tda5150.hpp"


static THD_WORKING_AREA(waBlinker, 304);	
static void  /*noreturn*/ blinker (void *arg)	
{
  (void)arg;					
  chRegSetThreadName("blinker");		
  
  while (true) {				
    palToggleLine(LINE_LED_GREEN);		
    chThdSleepMilliseconds(100);		
  }
}



int main (void)
{

  halInit();
  chSysInit();
  initHeap();	
  consoleInit();
  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, &blinker, NULL);

  TiedPins<2> tiedPin{
    {LINE_TDA5150_MOSI,
     PAL_MODE_ALTERNATE(AF_LINE_TDA5150_MOSI) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST},
    
    {LINE_FRAME_TX,
     PAL_MODE_ALTERNATE(AF_LINE_FRAME_TX) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST}
  };

  tiedPin.select(LINE_TDA5150_MOSI);
  consoleLaunch(); 
  
 
  chThdSleep(TIME_INFINITE);
}


