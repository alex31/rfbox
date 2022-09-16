#pragma once
#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"


namespace ModeExternal {
 void start(RfMode rfMode, uint32_t baud);
}
