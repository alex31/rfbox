#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "ssd1306.h"
#include "bboard.hpp"
#include "printf.h"
#include "hardwareConf.hpp"
#include "radio.hpp"
#include <algorithm>
#include "etl/string.h"

#define xstr(s) str(s)
#define str(s) #s
   

namespace {
  constexpr uint32_t  I2C_FAST_400KHZ_DNF3_R30NS_F30NS_PCLK80MHZ_TIMINGR = 0x00A02689;
  constexpr uint32_t stm32_cr1_dnf(uint8_t n)  {return ((n & 0x0f) << 8);}
  static constexpr I2CConfig i2ccfg_400 = {
    .timingr = I2C_FAST_400KHZ_DNF3_R30NS_F30NS_PCLK80MHZ_TIMINGR, // Refer to the STM32L4 reference manual
    .cr1 =  stm32_cr1_dnf(3U), // Digital noise filter disabled (timingr should be aware of that)
    .cr2 = 0 // Only the ADD10 bit can eventually be specified here (10-bit addressing mode)
  } ;

  const GFXfont font = Lato_Heavy_14;
  using OledLine = etl::string<oledWidth>;
  std::array<OledLine, oledHeight> oledScreen;

  THD_WORKING_AREA(waOledDisplay, 1024);	
  [[noreturn]] static void  oledDisplay (void *arg);
  void fillInit(void);
  void fillCalibrateRssi(void);
  void fillNoRfTx(void);
  void fillNoRfRx(void);
  void fillRxExternal(void);
  void fillTxExternal(void);
  void fillRxInternal(void);
  void fillTxInternal(void);
  void fillWhenError(void);
  void clearScreenBuffer(void);
}

namespace Oled {
  void start(void)
  {
    chThdCreateStatic(waOledDisplay, sizeof(waOledDisplay),
		      NORMALPRIO, &oledDisplay, nullptr);
  }
}


namespace {
  [[noreturn]] static void  oledDisplay (void *)	
  {
    Ope::Mode cmode = Ope::Mode::NONE;
    chRegSetThreadName("oledDisplay");		
    i2cStart(&SSD1306_I2CD, &i2ccfg_400);
    ssd1306_Init();
    
    while (true) {
      if ((Radio::radio != nullptr) and (Radio::radio->isInit())) {
	Radio::radio->getRssi();
	Radio::radio->getLnaGain();
      }
      ssd1306_Fill(BLACK);
      clearScreenBuffer();

      if (board.getError().empty()) {
	switch (cmode) {
	case Ope::Mode::NONE :
	  fillInit(); break;
	case Ope::Mode::NORF_TX :
	  fillNoRfTx(); break;
	case Ope::Mode::NORF_RX :
	  fillNoRfRx(); break;
	case Ope::Mode::RF_CALIBRATE_RSSI :
	  fillCalibrateRssi(); break;
	case Ope::Mode::RF_RX_EXTERNAL :
	  fillRxExternal(); break;
	case Ope::Mode::RF_TX_EXTERNAL :
	  fillTxExternal(); break;
	case Ope::Mode::RF_RX_INTERNAL :
	  fillRxInternal(); break;
	case Ope::Mode::RF_TX_INTERNAL :
	  fillTxInternal(); break;
	}
      } else {// error condition
	fillWhenError();
      }

      for (size_t li = 0; li < oledHeight; li++) {
	ssd1306_MoveCursor(0, ((li+1) * (SSD1306_HEIGHT / oledHeight)) - 1);
	ssd1306_WriteFmt(font, WHITE, "%s", oledScreen[li].c_str());
      }
      ssd1306_UpdateScreen();
      const bool longSplash = (cmode == Ope::Mode::NONE) and
	(RTCD1.rtc->BKP0R != warmBootSysRst);
      chThdSleepMilliseconds(longSplash ? 4000 : 500);
      RTCD1.rtc->BKP0R = warmBootWdg;
      cmode = board.getMode();
    }
  }


  void fillInit(void)
  {
    oledScreen = {
      RTCD1.rtc->BKP0R == warmBootWdg ?
      "*WATCHDOG Reset*" :
      "**** RF Box ****",
      xstr(GIT_TAG),
      "Enac/ELE Gorraz",
      "        Bustico"};
    if (not board.getError().empty()) {
      chsnprintf(oledScreen[1].begin(), oledScreen[1].capacity(),
		 board.getError().c_str());
    }
  }
  
  void fillNoRfTx(void)
  {
    oledScreen = {
      "     BE MCU     ",
      "module radio OFF",
      "mode USB-Serie  ",
      "*** Emission ***"};
  }
  
  void fillNoRfRx(void)
  {
    oledScreen = {
      "     BE MCU     ",
      "module radio OFF",
      "mode USB-Serie  ",
      "** Reception ** "};
  }

  void fillCalibrateRssi(void)
  {
    oledScreen = {
      "**** RF Box ****",
      "radio RX ON     ",
      "calibrate RSSI  ",
      ""};
  }
  
  void fillRxExternal(void)
  {
    chsnprintf(oledScreen[0].begin(), oledScreen[0].capacity(),
  	       "RX %lu Mhz", board.getFreq() / 1'000'000U);
    chsnprintf(oledScreen[1].begin(), oledScreen[1].capacity(),
  	       "Sortie Ext Av %.2f", board.getDioAvg());
    chsnprintf(oledScreen[2].begin(), oledScreen[2].capacity(),
  	       "Lna %d db", board.getLnaGain());
    chsnprintf(oledScreen[3].begin(), oledScreen[3].capacity(),
  	       "Rssi %d dbm", board.getRssi());
  }
  
  void fillTxExternal(void)
  {
    chsnprintf(oledScreen[0].begin(), oledScreen[0].capacity(),
  	       "TX %lu Mhz", board.getFreq() / 1'000'000U);
    chsnprintf(oledScreen[1].begin(), oledScreen[1].capacity(),
  	       "Source Externe");
    chsnprintf(oledScreen[2].begin(), oledScreen[2].capacity(),
  	       "P %d dbm", board.getTxPower());
    chsnprintf(oledScreen[3].begin(), oledScreen[3].capacity(),
  	       "Dio Avg = %.2f", board.getDioAvg());
  }
  
  void fillRxInternal(void)
  {
    chsnprintf(oledScreen[0].begin(), oledScreen[0].capacity(),
  	       "RX %lu Mhz", board.getFreq() / 1'000'000U);
    chsnprintf(oledScreen[1].begin(), oledScreen[1].capacity(),
  	       "BER %04d / 1000", std::min(uint16_t{1000}, board.getBer()));
    chsnprintf(oledScreen[2].begin(), oledScreen[2].capacity(),
  	       "Lna %d; %u baud", board.getLnaGain(), board.getBaud());
    chsnprintf(oledScreen[3].begin(), oledScreen[3].capacity(),
  	       "Rssi %d dbm", board.getRssi());
  }
  
  void fillTxInternal(void)
  {
    chsnprintf(oledScreen[0].begin(), oledScreen[0].capacity(),
  	       "TX %lu Mhz", board.getFreq() / 1'000'000U);
    chsnprintf(oledScreen[1].begin(), oledScreen[1].capacity(),
  	       "BER %u baud", board.getBaud());
    chsnprintf(oledScreen[2].begin(), oledScreen[2].capacity(),
  	       "Source Interne");
    chsnprintf(oledScreen[3].begin(), oledScreen[3].capacity(),
  	       "Puissance %d dbm", board.getTxPower());
  }
  
  void fillWhenError(void)
  {
    chsnprintf(oledScreen[0].begin(), oledScreen[0].capacity(),
  	       "ERREUR :");
    chsnprintf(oledScreen[1].begin(), oledScreen[1].capacity(),
  	       "%s", board.getError().c_str());
    chsnprintf(oledScreen[2].begin(), oledScreen[2].capacity(),
  	       "Mode=%s", Ope::toAscii(board.getMode()));
    chsnprintf(oledScreen[3].begin(), oledScreen[3].capacity(),
  	       "%s", xstr(GIT_TAG));
  }
  

  void clearScreenBuffer(void)
  {
    for (auto& l : oledScreen) {
      l.clear();
    }
  }
  //
  //  chsnprintf(oledScreen[0].begin(), oledScreen[0].capacity(),
  //	       "hello word");

}
