#include "modeTest.hpp"
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
  static  SerialConfig ftdiSerialConfig =  {
#if PACKET_EMISSION_READ_FROM_SERIAL
    .speed = baudLow,
#else
    .speed = baudHigh,
#endif
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN,
    //  | USART_CR2_TXINV | USART_CR2_SWAP;
    .cr3 = 0
  };
}

namespace ModeFsk {

  void start(RfMode rfMode, uint32_t baud)
  {
    ftdiSerialConfig.speed = baud;
    if (rfMode == RfMode::RX) {
      ftdiSerialConfig.cr2 |= USART_CR2_TXINV;
    } else {
#if PACKET_EMISSION_READ_FROM_SERIAL
      ftdiSerialConfig.cr2 |=  USART_CR2_SWAP;
#endif
    }
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
       switch (msg.status) {
       case SerialProtocol::Status::CRC_ERROR:
	 DebugTrace("CRC differ : L:0x%x != D:0x%x", msg.crc.local, msg.crc.distant);
	 break;
       case SerialProtocol::Status::SUCCESS:
	 // we wait for the rfm69 fifo to be empty
	 while(fskr->getFifoNotEmpty() == true) {
	   chThdSleepMilliseconds(1);
	 }
	 // serial msg len is len of just payload
	 // spi_frame len is for : payload + crc
	 msg.len += sizeof(msg.crc.distant);
	 // send payload
	 fskr->fifoWrite(&msg.len, msg.len - sizeof(msg.crc.distant));
	 // send crc
	 fskr->fifoWrite(&msg.crc.distant, sizeof(msg.crc.distant));
	 break;
       case SerialProtocol::Status::LEN_ERROR:
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
      // after this call, len is complete len : len of payload + crc
      msg.len -=  sizeof(msg.crc.local);
      memcpy(&msg.crc.local, &msg.payload[msg.len], sizeof(msg.crc.local));
      sendMsg(&SD_METEO, msg);
   }
  }
  
} // end of anonymous namespace
