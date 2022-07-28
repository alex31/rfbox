#pragma once
#include <ch.h>
#include <hal.h>

enum class DIPSWITCH {RFENABLE, FREQ, BER, BERBAUD, RXTX, PWRLVL, ALL};

namespace DIP {
  bool getDip(DIPSWITCH ds = DIPSWITCH::ALL);
  void start(void);
}
