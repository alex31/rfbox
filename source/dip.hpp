#pragma once
#include <ch.h>
#include <hal.h>

enum class DIPSWITCH {RXTX, POWER, FREQ, TEST, BAUD, ALL};

namespace DIP {
  msg_t getDip(DIPSWITCH ds = DIPSWITCH::ALL);
  void start(void);
}
