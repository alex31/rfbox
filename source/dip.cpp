#include "dip.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"


namespace {
  const ioline_t diplines[] = {LINE_DIP0_RFENABLE, LINE_DIP1_RXTX, LINE_DIP2_PWRLVL,
    LINE_DIP3_FREQ, LINE_DIP4_BER, LINE_DIP5_BERBAUD};

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
    return palReadLine(l) == PAL_LOW ? 1U : 0U;
  }

  inline uint8_t getAllDips()
  {
    return rl(LINE_DIP0_RFENABLE) | rl(LINE_DIP1_RXTX) << 1U |
      (rl(LINE_DIP2_PWRLVL) << 2U) | (rl(LINE_DIP3_FREQ) << 3U)  |
      (rl(LINE_DIP4_BER) << 4U) | (rl(LINE_DIP5_BERBAUD) << 5U);
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

