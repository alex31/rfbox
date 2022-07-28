#include "dip.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"


namespace {
  const ioline_t diplines[] = {LINE_DIP0_RFENABLE, LINE_DIP1_FREQ, LINE_DIP2_BER,
    LINE_DIP3_BERBAUD, LINE_DIP4_RXTX, LINE_DIP5_PWRLVL};

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
  inline bool rl(ioline_t l) {
    return palReadLine(l) == PAL_LOW ? false : true;
  }

  inline uint8_t getAllDips()
  {
    return rl(LINE_DIP0_RFENABLE) | rl(LINE_DIP1_FREQ) << 1U |
      (rl(LINE_DIP2_BER) << 2U) | (rl(LINE_DIP3_BERBAUD) << 3U)  |
      (rl(LINE_DIP4_RXTX) << 4U) | (rl(LINE_DIP5_PWRLVL) << 5U);
  }

  bool getDip(DIPSWITCH ds)
  {
    if (ds == DIPSWITCH::ALL) {
      return getAllDips();
    } else {
      const size_t idx = static_cast<size_t>(ds);
      chDbgAssert(idx <= static_cast<size_t>(DIPSWITCH::ALL), "out of bound");
      const bool dipLevel = rl(diplines[idx]);
      DebugTrace("dip %u level is %s", idx, dipLevel ? "ON" : "OFF");
      return dipLevel;
    }
  }



  void start(void)
  {
    chThdCreateStatic(waSurvey, sizeof(waSurvey), NORMALPRIO, &survey, NULL);
  }
  
}

