#pragma once
#include <ch.h>
#include <hal.h>
#include <array>
#include <utility>



namespace SerialProtocol {
  enum class Status {LEN_ERROR, CRC_ERROR, SUCCESS};
  struct Msg {
    size_t len;
    Status status;
    struct {
      uint16_t local;
      uint16_t distant;
    } crc;
    std::array<uint8_t, 255> payload;
  };
  Msg waitMsg(SerialDriver *sd);
}
