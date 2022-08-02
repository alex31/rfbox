#pragma once
#include <ch.h>
#include <hal.h>


namespace Buffer {
  enum class Mode {HiZ, TX, RX, INVERTED_TX, INVERTED_RX};
  void setMode(Mode mode);
}
