#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "tiedGpios.hpp"
#include "radio.hpp"


static constexpr sysinterval_t frameDuration =
  (2U * TIME_MS2I(1000U / DATAFRAME_FREQUENCY)) / 3U;

static constexpr SerialConfig repeaterCfg =  {
  .speed = UART_BAUD,
  .cr1 = 0,                     // pas de parité
  .cr2 = USART_CR2_STOP1_BITS
  | USART_CR2_LINEN, // 1 bit de stop, detection d'erreur de trame avancée
  .cr3 = 0           // pas de controle de flux hardware (CTS, RTS)
};

static THD_WORKING_AREA(waBlinker, 304);	
[[noreturn]] static void  blinker (void *arg)	
{
  (void)arg;					
  chRegSetThreadName("blinker");		
  
  while (true) {				
    palToggleLine(LINE_LED_GREEN);		
    chThdSleepMilliseconds(100);		
  }
}

static THD_WORKING_AREA(waAircast, 2048);	
[[noreturn]] static void  aircast (void *arg)	
{
  (void)arg;					
  chRegSetThreadName("aircast");
  uint8_t buffer[DATAFRAME_LEN];
  
  while (true) {				
    const size_t readLen = sdReadTimeout(&SD1, buffer, sizeof(buffer), frameDuration);
    if (readLen != DATAFRAME_LEN)
      continue;

    tda5150.startTransmit(TRANSMIT_CHAN_A | TRANSMIT_POWER_LEVEL_1 |
			  TRANSMIT_ENCODING_OFF | TRANSMIT_DATASYNC_OFF);
    sdWrite(&SD1, buffer, sizeof(buffer));
    tda5150.endTransmit();
  }
}



int main (void)
{

  halInit();
  chSysInit();
  initHeap();	
  consoleInit();
  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, &blinker, NULL);
  consoleLaunch(); 

  sdStart(&SD1, &repeaterCfg); 
  tda5150.init();
  tda5150.writeSfr(TdaSfr::TXCFG0,
		   {0x06, 0x25, 0x12, 0xA1, 
		    0xAB, 0x21, 0x7E, 0x5D, 0x0C, 0x40, 0x00, 0x00, 
		    0x10, 0x40, 0x00, 0x00, 0x10, 0x40, 0x00, 0x00, 
		    0x10, 0x00, 0xFC, 0xBB, 0xDE, 0x51, 0x48, 0x20, 
		    0x4C, 0x0B, 0x41, 0x00, 0x24, 0x58, 0xC0});
  
  chThdCreateStatic(waAircast, sizeof(waAircast), NORMALPRIO, &aircast, NULL);
  
  
 
  chThdSleep(TIME_INFINITE);
}


