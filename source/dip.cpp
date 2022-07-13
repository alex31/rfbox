#include "dip.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"


namespace {
  const ioline_t diplines[] = {LINE_DIP0_RXTX, LINE_DIP1_POWER, LINE_DIP2_FREQ,
    LINE_DIP3_MODE, LINE_DIP4_UNUSED };

  THD_WORKING_AREA(waSurvey, 304);
  void survey (void *arg)		
  {
    (void)arg;				
    chRegSetThreadName("survey");	
    
    const msg_t pattern = DIP::getDip();

    // if DIP switches are changed, make a restart
    while (true) {
      if (DIP::getDip() != pattern)
	systemReset();
      chThdSleepMilliseconds(20);
    }
  }

}



namespace DIP {
  inline msg_t rl(ioline_t l) {
    return palReadLine(l) == PAL_LOW ? 0U : 1U;
  }

  inline uint8_t getAllDips()
  {
    return rl(LINE_DIP0_RXTX) | rl(LINE_DIP1_POWER) << 1U |
      (rl(LINE_DIP2_FREQ) << 2U) | (rl(LINE_DIP3_MODE) << 3U)  |
      (rl(LINE_DIP4_UNUSED) << 4U);
  }

  msg_t getDip(DIPSWITCH ds)
  {
    if (ds == DIPSWITCH::ALL)
      return getAllDips();
    else
      return rl(diplines[static_cast<size_t>(ds)]);
  }



  void start(void)
  {
    chThdCreateStatic(waSurvey, sizeof(waSurvey), NORMALPRIO, &survey, NULL);
  }
  
}

