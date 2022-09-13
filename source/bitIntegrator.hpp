#pragma once
#include <bitset>

template<size_t N>
class Integrator
{
public:
  void push(bool v) {
    accum = accum + v - bs[index];
    bs[index] = v;
    index = (index + 1) % bs.size();
  }
  float getAvg(void) {
    return static_cast<float>(accum) / bs.size();
  }

private:
  std::bitset<N> bs = {};
  uint32_t accum = 0;
  size_t index = 0;
};
