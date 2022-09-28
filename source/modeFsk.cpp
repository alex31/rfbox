#include "modeTest.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"
#include "radio.hpp"
#include "hardwareConf.hpp"
#include "bboard.hpp"

namespace {
  //  THD_WORKING_AREA(waMsgStreamIn, 1280);
  //  THD_WORKING_AREA(waSurveyRestartRx, 512);
  
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
}

namespace ModeFsk {

  void start(RfMode rfMode, uint32_t baud)
  {
    meteoSerialConfig.speed = baud;
    // DIO is connected on UART1_TX
#if !defined(STM32F4xx_MCUCONF)
    if (rfMode == RfMode::RX) 
      meteoSerialConfig.cr2 |= USART_CR2_SWAP;
#endif
    if (rfMode == RfMode::RX) {
      sdStart(&SD_METEO, &meteoSerialConfig);
      // chThdCreateStatic(waMsgStreamIn, sizeof(waMsgStreamIn),
      // 			NORMALPRIO, &msgStreamIn, nullptr);
      // chThdCreateStatic(waSurveyRestartRx, sizeof(waSurveyRestartRx),
      // 			NORMALPRIO, &surveyRestartRx, nullptr);
     } 
  }
}

