#include "modeTest.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"
#include "radio.hpp"
#include "etl/string.h"
#include "hardwareConf.hpp"
#include "bboard.hpp"

constexpr uint8_t preambleByte = 0xff;

namespace {
  using ErrorString = etl::string<48>;
  THD_WORKING_AREA(waMsgStreamIn, 1280);
  
  static  SerialConfig meteoSerialConfig =  {
    .speed = 4800,
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN
#if DIO2_DIRECT && INVERT_UART_LEVEL && !defined(STM32F4xx_MCUCONF)
    | USART_CR2_TXINV | USART_CR2_RXINV
#endif
    ,
    .cr3 = 0
  };

   void msgStreamIn (void *);
  //    systime_t timoutTs = 0;
}



namespace ModeExternal {

  void start(RfMode rfMode, uint32_t baud)
  {
    meteoSerialConfig.speed = baud;
    // DIO is connected on UART1_TX
#if !defined(STM32F4xx_MCUCONF)
    if (rfMode == RfMode::RX) 
      meteoSerialConfig.cr2 |= USART_CR2_SWAP;
#endif
    sdStart(&SD_METEO, &meteoSerialConfig);
    if (rfMode == RfMode::RX) {
      chThdCreateStatic(waMsgStreamIn, sizeof(waMsgStreamIn),
			NORMALPRIO, &msgStreamIn, nullptr);
    } 
  }
}



namespace {
  

  void msgStreamIn (void *)		
  {
    chRegSetThreadName("");
    //    systime_t ts = chVTGetSystemTimeX();

    while (true) {
   }
  }
}
