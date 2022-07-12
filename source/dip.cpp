#include "dip.hpp"
#include "ch.h"
#include "hal.h"
#include <bit>


namespace DIP {
  inline msg_t rl(ioline_t l) {
    return palReadLine(l) == PAL_LOW ? 0U : 1U;
  }
  
  msg_t getDip0()
  {
    return palReadLine(LINE_DIP0_RXTX);
  }

  uint8_t getChannel()
  {
    return rl(LINE_DIP1_CH0) | (rl(LINE_DIP2_CH1) << 1) |
      (rl(LINE_DIP3_CH2) << 2)  | (rl(LINE_DIP4_CH3) << 3);
  }
}
