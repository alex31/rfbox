#pragma once
#include <bitset>

template<size_t N>
class Integrator
{
public:
  bool push(bool in) {
    const bool out =  bs[index];
    accum = accum + in - out;
    bs[index] = in;
    index = (index + 1) % bs.size();
    return out;
  }
  float getAvg(void) {
    return static_cast<float>(accum) / bs.size();
  }

private:
  std::bitset<N> bs = {};
  uint32_t accum = 0;
  size_t index = 0;
};

template<size_t N>
class DiffIntegrator
{
public:
  using funcb_t = void(*)(void);

  void setCb(float _thresholdRatio, funcb_t _cb) {
    thresholdRatio = _thresholdRatio;
    cb = _cb;
  };
  
  void activate(bool b) {
    active = b;
  }
  
  void push(bool in) {
    intOld.push(intNew.push(in));
    if (active and
	(cb != nullptr) and
	(getDifferential() > thresholdRatio)) {
      cb();
    }
  }

  float getDifferential(void) {
    const float ao = getAvgOld();
    const float an = getAvgNew();
    if (an < 0.25f) // "new" buffer should be reasonnably filled.
      return 1.0f;
    
    if (ao != 0.0f) {
      return an / ao;
    } else {
      return an == 0.0f ? 1.0f : 1000.0f;
    }
  }
  
  float getAvgNew(void) {
    return intNew.getAvg();
  }
  float getAvgOld(void) {
    return intOld.getAvg();
  }

private:
  Integrator<N> intOld, intNew;
  float thresholdRatio;
  funcb_t cb = nullptr;
  bool active = false;
};
