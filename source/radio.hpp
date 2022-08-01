#pragma once

#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"

namespace RADIO {
  extern Rfm69OokRadio radio;
  void init();
};
