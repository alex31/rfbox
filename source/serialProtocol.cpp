#include <ch.h>
#include <hal.h>
#include "serialProtocol.hpp"
#include "crcv1.h"

namespace SerialProtocol {
  constexpr auto time_500ms = TIME_MS2I(500);
  Msg waitMsg(SerialDriver *sd)
  {
    std::array<uint8_t, 2> sync = {};
    Msg msg = {.len = 0, .payload = {},
      .crc = {}, .status = {}
    };
    
    do {
      auto newByte = sdGetTimeout(sd, time_500ms);
      if (newByte < 0) {// timout
	msg.status = SerialProtocol::Status::TIMOUT;
	return msg;
      } else {
	sync[0] = sync[1];
	sync[1] = newByte;
      }
    } while ((sync[0] != 0xFE) or (sync[1] != 0xED));
    
    msg.len = sdGet(sd);
    if (msg.len == 0) {
      msg.status = SerialProtocol::Status::LEN_ERROR;
      return msg;
    }
    
    sdRead(sd, msg.payload.data(), msg.len);
    sdRead(sd, reinterpret_cast<uint8_t *>(&msg.crc.distant), sizeof(msg.crc.distant));
    crcReset(&CRCD1);
    msg.crc.local = crcCalc(&CRCD1, msg.payload.data(), msg.len);
    msg.status = msg.crc.local == msg.crc.distant ? SerialProtocol::Status::SUCCESS :
      SerialProtocol::Status::CRC_ERROR;
    return msg;
  }

  void sendMsg(SerialDriver *sd, const Msg& msg)
  {
    // header, len and payload
    sdWrite(sd, reinterpret_cast<const uint8_t *>(&msg.header),
	    sizeof(msg.header) + sizeof(msg.len) + msg.len);
    // crc
    sdWrite(sd, reinterpret_cast<const uint8_t *>(&msg.crc.local),
	    sizeof(msg.crc.local));
  }


  
}
