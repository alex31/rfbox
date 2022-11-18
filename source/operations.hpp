#pragma once
#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"

namespace Ope {
  enum class Status {OK, RFM69_ERROR, DATA_LINE_HOLD,
    INTERNAL_ERROR};

  enum class Mode {
    NONE, NORF_TX, NORF_RX, RF_CALIBRATE_RSSI,
    RF_RX_EXTERNAL_OOK, RF_TX_EXTERNAL_OOK,
    RF_RX_EXTERNAL_FSK, RF_TX_EXTERNAL_FSK,
    RF_RX_INTERNAL, RF_TX_INTERNAL
  };


  const char* toAscii(Mode opMode);
  const char* toAscii(Status status);
  
  void ajustSourceParams(Mode opMode);
  Ope::Status setMode(Mode opMode);
}
