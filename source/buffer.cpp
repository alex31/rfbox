#include <ch.h>
#include <hal.h>
#include "buffer.hpp"

namespace Buffer {
  void setMode(Mode mode)
  {
    palSetLine(LINE_BUFFER_TX_EN);
    palSetLine(LINE_BUFFER_RX_EN);
    palClearLine(LINE_BUFFER_INVERT); // Inverted on level_High
    chThdSleepMicroseconds(10);
    switch(mode) {
    case Mode::TX :
      palClearLine(LINE_BUFFER_TX_EN);
      break;
    case Mode::RX :
      palClearLine(LINE_BUFFER_RX_EN);
      break;
    case Mode::INVERTED_TX :
      palSetLine(LINE_BUFFER_INVERT);
      palClearLine(LINE_BUFFER_TX_EN);
      break;
    case Mode::INVERTED_RX :
      palSetLine(LINE_BUFFER_INVERT);
      palClearLine(LINE_BUFFER_RX_EN);
      break;
    case Mode::HiZ : break;
    }
  }
}
