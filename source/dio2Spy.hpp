#pragma once

#include "ch.h"
#include "hal.h"

namespace Dio2Spy {
  void start(ioline_t dio2Line);
  float getAverageLevel(void);
  float getInstantLevel(void);
}
