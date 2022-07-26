#pragma once
#include <ch.h>
#include <hal.h>


namespace GATE {
  enum class MODE {HiZ, TX, RX};
  void setMode(MODE mode);
}
