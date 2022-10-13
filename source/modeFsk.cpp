#include "modeFsk.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"
#include "radio.hpp"
#include "hardwareConf.hpp"
#include "bboard.hpp"
#include "serialProtocol.hpp"

namespace {
  THD_WORKING_AREA(waMsgRelay, 1280);
  [[noreturn]] void msgRelaySerialToSpi(void *arg);
  [[noreturn]] void msgRelaySpiToSerial(void *arg);
  THD_WORKING_AREA(waSurveyRestartRx, 512);
  void surveyRestartRx (void *);
  
  static  SerialConfig ftdiSerialConfig =  {
    .speed = baudRates[+BitRateIndex::Low],
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN,
    .cr3 = 0
  };

  virtual_timer_t vtWatchDog;
  volatile bool shouldRestartRx = false;
  auto restartRxCb = [](ch_virtual_timer *, void *) {
    shouldRestartRx = true;
  };
}

namespace ModeFsk {

  void start(RfMode rfMode, uint32_t baud, Source source)
  {
    ftdiSerialConfig.speed = baud;
    if (rfMode == RfMode::RX) {
      ftdiSerialConfig.cr2 |= USART_CR2_TXINV;
    } else {
      chDbgCheck(source != Source::NONE);
      if (source == ModeFsk::Source::SERIAL)
	ftdiSerialConfig.cr2 |=  USART_CR2_SWAP;
    }
    sdStart(&SD_METEO, &ftdiSerialConfig);
    if (rfMode == RfMode::RX) {
      chThdCreateStatic(waMsgRelay, sizeof(waMsgRelay),
       			NORMALPRIO, &msgRelaySpiToSerial, nullptr);
      chThdCreateStatic(waSurveyRestartRx, sizeof(waSurveyRestartRx),
			NORMALPRIO, &surveyRestartRx, nullptr);
    } else  if (rfMode == RfMode::TX) {
      chThdCreateStatic(waMsgRelay, sizeof(waMsgRelay),
       			NORMALPRIO, &msgRelaySerialToSpi, nullptr);
    } else {
      chSysHalt("rfMode neither TX or RX");
    }
  } 

}

namespace {

  void msgRelaySerialToSpi(void *)
  {
    chRegSetThreadName("Serial to SPI");
    auto fskr = static_cast<Rfm69FskRadio *>(Radio::radio);
    
    while(true) {
      SerialProtocol::Msg msg = SerialProtocol::waitMsg(&SD_METEO);
      //             DebugTrace("+++++++ Ser2Spi msg ++++++++");
      switch (msg.status) {
      case SerialProtocol::Status::CRC_ERROR:
	//	DebugTrace("CRC differ : L:0x%x != D:0x%x", msg.crc.local, msg.crc.distant);
	break;
      case SerialProtocol::Status::SUCCESS:
	//	 // we wait for the rfm69 fifo to be empty
	//	DebugTrace("+++++++ Ser2Spi Ok ++++++++");
	while(fskr->getFifoNotEmpty() == true) {
	  chThdSleepMilliseconds(1);
	}
	// serial msg len is len of just payload
	// spi_frame len is for : payload + crc
	//	 DebugTrace("+++++++ Ser2Spi fifo W ++++++++");
	msg.len += sizeof(msg.crc.distant);
	//	DebugTrace("fifo send len = %u", msg.len);
	// send payload
	fskr->fifoWrite(&msg.len, msg.len + sizeof(msg.len) - sizeof(msg.crc.distant));
	// send crc
	fskr->fifoWrite(&msg.crc.distant, sizeof(msg.crc.distant));
	break;
      case SerialProtocol::Status::LEN_ERROR:
      case SerialProtocol::Status::TIMOUT:
	break;
      }
    }
  }
  
  void msgRelaySpiToSerial(void *)
  {
    chRegSetThreadName("SPI to Serial");
    auto fskr = static_cast<Rfm69FskRadio *>(Radio::radio);
    SerialProtocol::Msg msg;

    chVTSetContinuous(&vtWatchDog, TIME_MS2I(1500), restartRxCb, nullptr);
    
    while(true) {
      // we wait for the rfm69 fifo to filled with a complete message
      while(fskr->getPayloadReady() == false) {
	chThdSleepMilliseconds(1);
      }
      chVTSetContinuous(&vtWatchDog, TIME_MS2I(1500), restartRxCb, nullptr);
      fskr->fifoRead(msg.payload.data(), &msg.len);
      DebugTrace("payload ready = %u", fskr->getPayloadReady());
      
      // after this call, len is complete len : len of payload + crc
      //   DebugTrace("+++++++ Spi2Ser msg len= %u ++++++++", msg.len);
      if (msg.len > sizeof(msg.crc.local)) {
	msg.len -=  sizeof(msg.crc.local);
	memcpy(&msg.crc.local, &msg.payload[msg.len], sizeof(msg.crc.local));
	sendMsg(&SD_METEO, msg);
      }
    }
  }
  
 void surveyRestartRx (void *)		
  {
    chRegSetThreadName("survey Restart Rx");
    while (true) {
      if (shouldRestartRx) {
	Radio::radio->forceRestartRx();
	shouldRestartRx = false;
	DebugTrace("restart Rx");
      }
      chThdSleepMilliseconds(100);
    }
  }
  



} // end of anonymous namespace
