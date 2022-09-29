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
  
  static  SerialConfig ftdiSerialConfig =  {
    .speed = baudHigh,
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN,
    .cr3 = 0
  };
}

namespace ModeFsk {

  void start(RfMode rfMode, uint32_t baud)
  {
    ftdiSerialConfig.speed = baud;
    // DIO is connected on UART1_TX
    sdStart(&SD_METEO, &ftdiSerialConfig);
    if (rfMode == RfMode::RX) {
      // chThdCreateStatic(waMsgStreamIn, sizeof(waMsgStreamIn),
      // 			NORMALPRIO, &msgStreamIn, nullptr);
      // chThdCreateStatic(waSurveyRestartRx, sizeof(waSurveyRestartRx),
      // 			NORMALPRIO, &surveyRestartRx, nullptr);
    }
  } 

}


