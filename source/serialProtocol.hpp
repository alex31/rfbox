#pragma once
#include <ch.h>
#include <hal.h>
#include <array>
#include <utility>
#include "stdutil.h"


namespace SerialProtocol {
  enum class Status {LEN_ERROR, CRC_ERROR, SUCCESS};
  struct Msg {
    struct {
      const uint16_t header = SWAP_ENDIAN16(0xFEED);
      uint8_t len;
      std::array<uint8_t, 255> payload;
      struct {
	uint16_t local;
	uint16_t distant;
      } crc;
    } __attribute__((packed));
    Status status;
  };
  Msg waitMsg(SerialDriver *sd);
  void sendMsg(SerialDriver *sd, const Msg& msg);
}
