#include "modeTest.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"
#include "radio.hpp"
#include "hardwareConf.hpp"
#include "bboard.hpp"
#include "serialProtocol.hpp"
#include "crcv1.h"

namespace {
  THD_WORKING_AREA(waMsgRelay, 1280);
  [[noreturn]] void msgRelaySerialToSpi(void *arg);
  [[noreturn]] void msgRelaySpiToSerial(void *arg);
  static  SerialConfig ftdiSerialConfig =  {
#if PACKET_EMISSION_READ_FROM_SERIAL
    .speed = baudLow,
#else
    .speed = baudHigh,
#endif
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN,
    .cr3 = 0
  };
}

namespace ModeFsk {

  void start(RfMode rfMode, uint32_t )
  {
    if (rfMode == RfMode::RX) {
      ftdiSerialConfig.cr2 |= USART_CR2_TXINV;
    } else {
#if PACKET_EMISSION_READ_FROM_SERIAL
      ftdiSerialConfig.cr2 |=  USART_CR2_SWAP;
#endif
    }
    crcInit();
    crcStart(&CRCD1, &crcCfgModbus);
    sdStart(&SD_METEO, &ftdiSerialConfig);
    if (rfMode == RfMode::RX) {
      chThdCreateStatic(waMsgRelay, sizeof(waMsgRelay),
       			NORMALPRIO, &msgRelaySpiToSerial, nullptr);
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
      //       DebugTrace("+++++++ Ser2Spi msg ++++++++");
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
    
    while(true) {
      // we wait for the rfm69 fifo to filled with a complete message
      while(fskr->getPayloadReady() == false) {
	chThdSleepMilliseconds(1);
      }
      fskr->fifoRead(msg.payload.data(), &msg.len);
      //      DebugTrace("payload ready = %u", fskr->getPayloadReady());
      
      // after this call, len is complete len : len of payload + crc
      //   DebugTrace("+++++++ Spi2Ser msg len= %u ++++++++", msg.len);
      if (msg.len > sizeof(msg.crc.local)) {
	msg.len -=  sizeof(msg.crc.local);
	memcpy(&msg.crc.local, &msg.payload[msg.len], sizeof(msg.crc.local));
	sendMsg(&SD_METEO, msg);
      }
    }
  }
  
  // void msgRelaySerialToSpi(void *)
  // {
  //   chRegSetThreadName("Fake Serial to SPI");
  //   auto fskr = static_cast<Rfm69FskRadio *>(Radio::radio);
  //   const uint8_t buffer[9] = {8, 10, 20, 30, 40, 50, 60, 70, 80};
    
  //   while(true) {
  //     while(fskr->getFifoNotEmpty() == true) {
  // 	chThdSleepMilliseconds(1);
  //     }
  //     fskr->fifoWrite(buffer, sizeof(buffer));
  //     chThdSleepMilliseconds(200);
  //   }
  // }
  
  // void msgRelaySpiToSerial(void *)
  // {
  //  chRegSetThreadName("Fake SPI to Serial");
  //  auto fskr = static_cast<Rfm69FskRadio *>(Radio::radio);
  //  uint8_t buffer[64] = {};

  //  while(true) {
  //    // we wait for the rfm69 fifo to filled with a complete message
  //    while(fskr->getPayloadReady() == false) {
  //      chThdSleepMilliseconds(1);
  //    }
  //    uint8_t len;
  //    fskr->fifoRead(buffer, &len);

  //    DebugTrace("+++++++ Spi2Ser msg len= %u ++++++", len);
     
  //    chprintf(chp, "buffer = ");
  //    for (size_t i=0; i<len; i++) {
  //      chprintf(chp, "%u, ", buffer[i]);
  //    }
  //    chprintf(chp, "\r\n");
  //  }
  // }



} // end of anonymous namespace
