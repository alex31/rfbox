#include <ch.h>
#include <hal.h>
#include <string>
#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "encoderTimer.hpp"	

uint8_t binaryToGray(uint8_t num);
uint8_t grayToBinary(uint8_t num);

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
  while (not lptim1d.zeroSetDone()) {
    chThdSleepMilliseconds(100);
    palToggleLine(LINE_NOT_ZEROED);
  }

  palClearLine(LINE_NOT_ZEROED);
  uint8_t lastCode = 0xFF;
  
  while (true) {				
    auto [u, cnt] = lptim1d.getCnt();
    if (u) {
      const uint8_t angle = (cnt * 36 / 2048);
      const uint8_t grayCode = binaryToGray(angle);
      if (lastCode != grayCode) {
	lastCode = grayCode;
	uint8_t portGrayCode = grayCode << 2;
	// dispatch 6 bits code on Port A : 1,3,4,5,6,7
	(portGrayCode |= ((portGrayCode & 0b100) >> 1)) &= 0b11111010;
	std::string binGray(binary_fmt(grayCode, 8));
	std::string binPort(binary_fmt(portGrayCode, 8));
	// atomic 6 bits gray code update
	palWriteGroup(GPIOA, 0b11111010, 0, portGrayCode); 
	DebugTrace("lptim1 CNT = %u [%u] [g:0x%x -> 0x%x] {%s -> %s}",
		   cnt, angle, grayCode, portGrayCode,
		   binGray.c_str(), binPort.c_str());

	// This is not atomic
	// palWriteLine(LINE_GRAY_BIT0, (grayCode & (1 << 0)) ? PAL_HIGH : PAL_LOW);
	// palWriteLine(LINE_GRAY_BIT1, (grayCode & (1 << 1)) ? PAL_HIGH : PAL_LOW);
	// palWriteLine(LINE_GRAY_BIT2, (grayCode & (1 << 2)) ? PAL_HIGH : PAL_LOW);
	// palWriteLine(LINE_GRAY_BIT3, (grayCode & (1 << 3)) ? PAL_HIGH : PAL_LOW);
	// palWriteLine(LINE_GRAY_BIT4, (grayCode & (1 << 4)) ? PAL_HIGH : PAL_LOW);
	// palWriteLine(LINE_GRAY_BIT5, (grayCode & (1 << 5)) ? PAL_HIGH : PAL_LOW);
      }
    }
    chThdSleepMilliseconds(1);
  }
}



int main (void)
{

  halInit();
  chSysInit();
  initHeap();	
  consoleInit();
  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, &blinker, NULL);
  chThdCreateStatic(waLptim1, sizeof(waLptim1), NORMALPRIO - 1, &lptim1, NULL);

  palEnableLineEvent(LINE_ENCODER_ZERO, PAL_EVENT_MODE_FALLING_EDGE);
  palSetLineCallback(LINE_ENCODER_ZERO,
		     [] (void *) {
		       lptim1d.reset();
		     },
		     NULL);
  
  consoleLaunch(); 

  
  chThdSleep(TIME_INFINITE);
}


uint8_t binaryToGray(uint8_t num)
{
    return num ^ (num >> 1); // The operator >> is shift right. The operator ^ is exclusive or.
}

// This function converts a reflected binary Gray code number to a binary number.
uint8_t grayToBinary(uint8_t num)
{
    uint8_t mask = num;
    while (mask) {           // Each Gray code bit is exclusive-ored with all more significant bits.
        mask >>= 1;
        num   ^= mask;
    }
    return num;
}
