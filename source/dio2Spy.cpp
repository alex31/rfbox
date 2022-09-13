#include "dio2Spy.hpp"
#include "bitIntegrator.hpp"
#include "bboard.hpp"

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
    const float avg = integ.getAvg();
    board.setDioAvg(avg);
    return avg;
  }
}
