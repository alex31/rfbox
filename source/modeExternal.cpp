#include "modeTest.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"
#include "radio.hpp"
#include "hardwareConf.hpp"
#include "bboard.hpp"
#include "dio2Spy.hpp"
#include "serialProtocol.hpp"

constexpr uint8_t preambleByte = 0xff;

namespace {
  THD_WORKING_AREA(waMsgStreamIn, 1280);
  THD_WORKING_AREA(waSurveyRestartRx, 512);
  
  static  SerialConfig meteoSerialConfig =  {
    .speed = baudRates[+BitRateIndex::Low],
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN
#if INVERT_OOK_MODUL 
    | USART_CR2_TXINV | USART_CR2_RXINV
#endif
    ,
    .cr3 = 0
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

    if (rfMode == RfMode::RX) {
      meteoSerialConfig.cr2 |= USART_CR2_SWAP;
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
  
} // end of namespace ModeExternal


//    systime_t ts = chVTGetSystemTimeX();
namespace {
  

  void msgStreamIn (void *)		
  {
    chRegSetThreadName("Decode Meteo Msg");
    
    auto lambda = [](ch_virtual_timer *, void *) {
      shouldRestartRx = true;
    };
    chVTSetContinuous(&vtWatchDog, TIME_MS2I(1500), lambda, nullptr);

    while (true) {
      const SerialProtocol::Msg msg = SerialProtocol::waitMsg(&SD_METEO);
      switch (msg.status) {
      case SerialProtocol::Status::CRC_ERROR:
	DebugTrace("CRC differ : L:0x%x != D:0x%x", msg.crc.local, msg.crc.distant);
	break;
      case SerialProtocol::Status::SUCCESS:
	chVTSetContinuous(&vtWatchDog, TIME_MS2I(1500), lambda, nullptr);
	break;
      case SerialProtocol::Status::TIMOUT:
      case SerialProtocol::Status::LEN_ERROR:
	break;
      }
    }
  }

 void surveyRestartRx (void *)		
  {
    chRegSetThreadName("survey Restart Rx");
    Dio2Spy::setCb(3.0f, [] {
      Radio::radio->forceRestartRx();
      Dio2Spy::activate(false);
      DebugTrace("********* forceRestartRx ratio = %.2f", Dio2Spy::getDifferential());
    });

    Dio2Spy::activate(false);
    while (true) {
      if (shouldRestartRx) {
	Radio::radio->forceRestartRx();
	shouldRestartRx = false;
	Dio2Spy::activate(true);
	DebugTrace("activate");
      }
      chThdSleepMilliseconds(100);
    }
  }
  

  
}
