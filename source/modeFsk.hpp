#pragma once
#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"


namespace ModeFsk {
  enum class Source {NONE, SERIAL, USB_CDC};
  void start(RfMode rfMode, uint32_t baud, Source source);
}
