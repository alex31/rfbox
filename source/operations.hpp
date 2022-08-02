#pragma once
#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"

namespace Ope {
  enum class Status {OK, RFM69_ERROR, DATA_LINE_HOLD,
    INTERNAL_ERROR};

  enum class Mode {
    NONE, NORF_TX, NORF_RX, RF_CALIBRATE_RSSI,
    RF_RX_EXTERNAL, RF_TX_EXTERNAL, RF_RX_INTERNAL,
    RF_TX_INTERNAL
  };
  
  Ope::Status setMode(Mode opMode, 
		      uint32_t frequencyCarrier,
		      int8_t amplificationLevelDb);
}
