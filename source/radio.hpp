#pragma once

#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"
#include "operations.hpp"

namespace Radio {
  extern Rfm69BaseRadio *radio;
  void init(Ope::Mode opMode);
};
