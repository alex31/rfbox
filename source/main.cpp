#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "ttyConsole.hpp"	

int main (void)
{

  halInit();
  chSysInit();

#ifndef NOSHELL
  initHeap();
  consoleInit();
  consoleLaunch(); 
#endif
  

  chThdSleep(TIME_INFINITE);
  while(true) {};
}


