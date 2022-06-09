#pragma once

#include <ch.h>
#include <hal.h>
#include "etl/map.h"
#include  "stdutil.h"


template<size_t N>
class TiedPins {
private:
  static constexpr ioline_t gpioAll = 0xffffffff;
  
public:
  struct OneGpio {
    ioline_t line;
    iomode_t mode;
  };
  
  TiedPins(const std::initializer_list<OneGpio>& gpios) {populate(gpios);highZ(gpioAll);}
  void select(const ioline_t line);
private:
  etl::map<ioline_t, iomode_t, N> modeByline;
  void highZ(const ioline_t line);
  void setAltMode(const ioline_t line);
  void populate(const std::initializer_list<OneGpio>& gpios);
};


template<size_t N>
void TiedPins<N>::populate(const std::initializer_list<OneGpio>& gpios) 
{
    for (const auto [l, m] : gpios) {
        modeByline[l] = m;
    }
}

template<size_t N>
void TiedPins<N>::highZ(const ioline_t line)
{
    constexpr static auto doHighZ = [](const ioline_t l) {
      //        DebugTrace("high Z line 0x%x\n", l);
	palSetLineMode(l, PAL_STM32_MODE_ANALOG);
    };

    if (line != gpioAll) {
        doHighZ(line);
    } else {
        for (auto const& [l, m] : modeByline) 
             doHighZ(l);
    }
}

template<size_t N>
void TiedPins<N>::setAltMode(const ioline_t line)
{
   assert(modeByline.contains(line)); 
   //   DebugTrace("set line 0x%x to mode 0x%x\n", line, modeByline[line]);
   palSetLineMode(line, modeByline[line]);
}

template<size_t N>
void TiedPins<N>::select(const ioline_t line)
{
    for (auto const& [l, m] : modeByline) {
        if (l != line)
         highZ(l);
    }
    setAltMode(line);
}

