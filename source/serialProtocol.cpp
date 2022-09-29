#include <ch.h>
#include <hal.h>
#include "serialProtocol.hpp"
#include "crcv1.h"

namespace SerialProtocol {

  Msg waitMsg(SerialDriver *sd)
  {
    std::array<uint8_t, 2> sync = {};
    Msg msg = {.len = 0, .status = SerialProtocol::Status::LEN_ERROR,
      .crc = {}, .payload = {}};
    
    do {
      sync[0] = sync[1];
      sync[1] = sdGet(sd);
    } while ((sync[0] != 0xFE) or (sync[1] != 0xED));
    
    msg.len = sdGet(sd);
    if (msg.len == 0)
      return msg;
    
    sdRead(sd, msg.payload.data(), msg.len);
    sdRead(sd, reinterpret_cast<uint8_t *>(&msg.crc.distant), sizeof(msg.crc.distant));
    crcReset(&CRCD1);
    msg.crc.local = crcCalc(&CRCD1, msg.payload.data(), msg.len);
    msg.status = msg.crc.local == msg.crc.distant ? SerialProtocol::Status::SUCCESS :
      SerialProtocol::Status::CRC_ERROR;
    return msg;
  }
  
}
