#include "dio2Spy.hpp"
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


namespace {
  Integrator<10000> integ;
  ioline_t dio2Line;
  THD_WORKING_AREA(waSurvey, 512);
  void survey (void *arg)		
  {
    (void)arg;				
    chRegSetThreadName("DIO2 change survey");	
    
    while (true) {
      integ.push(palReadLine(dio2Line));
      chThdSleepMicroseconds(100);
    }
  }

}

namespace Dio2Spy {
  void start(ioline_t _dio2Line)
  {
    dio2Line = _dio2Line;
    chThdCreateStatic(waSurvey, sizeof(waSurvey),
		      LOWPRIO, &survey, NULL);
  }

  float getAverageLevel(void)
  {
    return integ.getAvg();
  }
}
