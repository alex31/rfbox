#include "modeTest.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"
#include "radio.hpp"
#include "etl/string.h"
#include "hardwareConf.hpp"
#include "bboard.hpp"
#include "crcv1.h"
#include "dio2Spy.hpp"

constexpr uint8_t preambleByte = 0xff;

namespace {
  using ErrorString = etl::string<48>;
  THD_WORKING_AREA(waMsgStreamIn, 1280);
  THD_WORKING_AREA(waSurveyRestartRx, 512);
  
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

  static const CRCConfig crcCfgModbus = {
    .poly_size = 16,
    .poly = 0x8005,
    .initial_val = 0xffff,
    .final_val = 0x0000,
    .reflect_data = true,
    .reflect_remainder = true
  };

  virtual_timer_t vtWatchDog;
  volatile bool shouldRestartRx = false;
  
  void msgStreamIn (void *);
  void surveyRestartRx (void *);
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
    if (rfMode == RfMode::RX) {
      crcInit();
      crcStart(&CRCD1, &crcCfgModbus);
      chVTSet(&vtWatchDog, TIME_S2I(3), [](ch_virtual_timer *, void *) {
	shouldRestartRx = true;
      },
	  nullptr);
      sdStart(&SD_METEO, &meteoSerialConfig);
      chThdCreateStatic(waMsgStreamIn, sizeof(waMsgStreamIn),
			NORMALPRIO, &msgStreamIn, nullptr);
     chThdCreateStatic(waSurveyRestartRx, sizeof(waSurveyRestartRx),
			NORMALPRIO, &surveyRestartRx, nullptr);
     } 
  }
}


//    systime_t ts = chVTGetSystemTimeX();
namespace {
  

  void msgStreamIn (void *)		
  {
    chRegSetThreadName("Decode Meteo Msg");

    while (true) {
      std::array<uint8_t, 2> sync = {};
      uint8_t len = {};
      std::array<uint8_t, 255> payload = {};
      uint16_t distantCrc, localCrc;
      
      do {
	sync[0] = sync[1];
	sync[1] = sdGet(&SD_METEO);
	//DebugTrace(".");
      } while ((sync[0] != 0xFE) or (sync[1] != 0xED));

      len = sdGet(&SD_METEO);
      sdRead(&SD_METEO, payload.data(), len);
      sdRead(&SD_METEO, reinterpret_cast<uint8_t *>(&distantCrc), sizeof(distantCrc));
      crcReset(&CRCD1);
      localCrc = crcCalc(&CRCD1, payload.data(), len);
      if (localCrc != distantCrc) {
	DebugTrace("CRC differ : L:0x%x != D:0x%x", localCrc, distantCrc);
      } else {
	chVTReset(&vtWatchDog);
	chVTSet(&vtWatchDog, TIME_S2I(3), [](ch_virtual_timer *, void *) {
	  shouldRestartRx = true;
	},
	  nullptr);
      }
    }
  }

 void surveyRestartRx (void *)		
  {
    chRegSetThreadName("survey Restart Rx");

    while (true) {
      if (shouldRestartRx) {
	shouldRestartRx = false;
	Radio::radio.forceRestartRx();
      // 	while(true) {
      // 	  chThdSleepMicroseconds(100);
      // 	  if (Dio2Spy::getInstantLevel() > 0.9f) {
      // 	    Radio::radio.forceRestartRx();
      // 	    break;
      // 	  }
      // }
      chThdSleepMilliseconds(100);
    }
  }
  
  }

}