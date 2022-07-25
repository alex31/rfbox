#pragma once
#include <ch.h>
#include <hal.h>

enum class DIPSWITCH {RFENABLE, RXTX, PWRLVL, FREQ, BER, BERBAUD, ALL};

namespace DIP {
  msg_t getDip(DIPSWITCH ds = DIPSWITCH::ALL);
  void start(void);
}
