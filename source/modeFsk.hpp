#pragma once
#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"


namespace ModeFsk {
 void start(RfMode rfMode, uint32_t baud);
}
