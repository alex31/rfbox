#include <ch.h>
#include <hal.h>
#include "notGate.hpp"

namespace GATE {
  void setMode(MODE mode)
  {
    palSetLine(LINE_NOTGATE_TX_EN);
    palSetLine(LINE_NOTGATE_RX_EN);
    chThdSleepMicroseconds(10);
    switch(mode) {
    case MODE::TX : palClearLine(LINE_NOTGATE_TX_EN); break;
    case MODE::RX : palClearLine(LINE_NOTGATE_RX_EN); break;
    case MODE::HiZ : break;
    }
  }
}
