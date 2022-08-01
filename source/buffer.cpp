#include <ch.h>
#include <hal.h>
#include "buffer.hpp"

namespace BUFFER {
  void setMode(MODE mode)
  {
    palSetLine(LINE_BUFFER_TX_EN);
    palSetLine(LINE_BUFFER_RX_EN);
    palSetLine(LINE_BUFFER_INVERT); // Inverted on level_Low
    chThdSleepMicroseconds(10);
    switch(mode) {
    case MODE::TX :
      palClearLine(LINE_BUFFER_TX_EN);
      break;
    case MODE::RX :
      palClearLine(LINE_BUFFER_RX_EN);
      break;
    case MODE::INVERTED_TX :
      palClearLine(LINE_BUFFER_INVERT);
      palClearLine(LINE_BUFFER_TX_EN);
      break;
    case MODE::INVERTED_RX :
      palClearLine(LINE_BUFFER_INVERT);
      palClearLine(LINE_BUFFER_RX_EN);
      break;
    case MODE::HiZ : break;
    }
  }
}
