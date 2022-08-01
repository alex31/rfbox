#pragma once
#include <ch.h>
#include <hal.h>


namespace BUFFER {
  enum class MODE {HiZ, TX, RX, INVERTED_TX, INVERTED_RX};
  void setMode(MODE mode);
}
