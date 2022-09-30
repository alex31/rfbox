#pragma once
#include <ch.h>
#include <hal.h>

enum class DIPSWITCH {RFENABLE, FREQ, BER, BAUD_MODUL, RXTX, PWRLVL};

namespace DIP {
  bool getDip(DIPSWITCH ds);
  void start(void);
}
