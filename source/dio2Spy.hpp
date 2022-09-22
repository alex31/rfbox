#pragma once

#include "ch.h"
#include "hal.h"
#include "bitIntegrator.hpp"

namespace Dio2Spy {
  void start(ioline_t dio2Line);
  float getAverageLevel(void);
  float getInstantLevel(void);
  void setCb(float thresholdRatio, DiffIntegrator<32>::funcb_t cb);
  void activate(bool b);
}
