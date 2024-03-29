#include "dio2Spy.hpp"
#include "bitIntegrator.hpp"
#include "bboard.hpp"

namespace {
  Integrator<10016> integ1S;
  DiffIntegrator<32> integ6Ms;
  ioline_t dio2Line;
  THD_WORKING_AREA(waSurvey, 512);
  bool started = false;
  
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
    if (not started) {
      chThdCreateStatic(waSurvey, sizeof(waSurvey),
			HIGHPRIO, &survey, nullptr);
      started = true;
    }
  }

  float getAverageLevel(void)
  {
    const float avg = integ1S.getAvg();
    board.setDioAvg(avg);
    return avg;
  }
  
  float getDifferential(void)
  {
    return integ6Ms.getDifferential();
  }
  
  void setCb(float thresholdRatio, DiffIntegrator<32>::funcb_t cb)
  {
    integ6Ms.setCb(thresholdRatio, cb);
  };

  void activate(bool b)
  {
    integ6Ms.activate(b);
  }
 
}
