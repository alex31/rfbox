#include "dio2Spy.hpp"
#include "bitIntegrator.hpp"
#include "bboard.hpp"

namespace {
  Integrator<10000> integ1S;
  Integrator<64> integ6Ms;
  ioline_t dio2Line;
  THD_WORKING_AREA(waSurvey, 512);
  void survey (void *arg)		
  {
    (void)arg;				
    chRegSetThreadName("DIO2 change survey");	
    
    while (true) {
      const uint32_t level = palReadLine(dio2Line);
      integ1S.push(level);
      integ6Ms.push(level);
      chThdSleepMicroseconds(100);
    }
  }

}

namespace Dio2Spy {
  void start(ioline_t _dio2Line)
  {
    dio2Line = _dio2Line;
    chThdCreateStatic(waSurvey, sizeof(waSurvey),
		      HIGHPRIO, &survey, NULL);
  }

  float getAverageLevel(void)
  {
    const float avg = integ1S.getAvg();
    board.setDioAvg(avg);
    return avg;
  }

  float getInstantLevel(void)
  {
    const float avg = integ6Ms.getAvg();
    return avg;
  }
}
