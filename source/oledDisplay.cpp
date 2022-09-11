#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "ssd1306.h"
#include "bboard.hpp"
#include "etl/string.h"
#include "printf.h"
#include "hardwareConf.hpp"

#define xstr(s) str(s)
#define str(s) #s
   

namespace {
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
    chThdCreateStatic(waOledDisplay, sizeof(waOledDisplay), NORMALPRIO, &oledDisplay, nullptr);
  }
}


namespace {
  [[noreturn]] static void  oledDisplay (void *)	
  {
    chRegSetThreadName("oledDisplay");		
    
    ssd1306_Init();
    
    while (true) {
      ssd1306_Fill(BLACK);
      clearScreenBuffer();

      if (board.getError().empty()) {
	switch (board.getMode()) {
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
      chThdSleepMilliseconds(500);
    } 
  }


  void fillInit(void)
  {
    oledScreen = {
      "**** RF Box ****",
      " initialisation ",
      xstr(GIT_TAG),
      "Enac Grz Bto ELS"};
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
    chsnprintf(oledScreen[0].begin(), oledScreen[0].size(),
  	       "RX %lu Mhz", board.getFreq());
    chsnprintf(oledScreen[1].begin(), oledScreen[1].size(),
  	       "Source Externe");
    chsnprintf(oledScreen[2].begin(), oledScreen[2].size(),
  	       "Lna %d db", board.getLnaGain());
    chsnprintf(oledScreen[3].begin(), oledScreen[3].size(),
  	       "Rssi %d dbm", board.getRssi());
  }
  
  void fillTxExternal(void)
  {
    chsnprintf(oledScreen[0].begin(), oledScreen[0].size(),
  	       "TX %lu Mhz", board.getFreq());
    chsnprintf(oledScreen[1].begin(), oledScreen[1].size(),
  	       "Source Externe");
    chsnprintf(oledScreen[2].begin(), oledScreen[2].size(),
  	       "P %d dbm", board.getTxPower());
  }
  
  void fillRxInternal(void)
  {
    chsnprintf(oledScreen[0].begin(), oledScreen[0].size(),
  	       "RX %lu Mhz", board.getFreq());
    chsnprintf(oledScreen[1].begin(), oledScreen[1].size(),
  	       "BER %04d / 1000", board.getBer());
    chsnprintf(oledScreen[2].begin(), oledScreen[2].size(),
  	       "Lna %d db", board.getLnaGain());
    chsnprintf(oledScreen[3].begin(), oledScreen[3].size(),
  	       "Rssi %d dbm", board.getRssi());
  }
  
  void fillTxInternal(void)
  {
    chsnprintf(oledScreen[0].begin(), oledScreen[0].size(),
  	       "TX %lu Mhz", board.getFreq());
    chsnprintf(oledScreen[1].begin(), oledScreen[1].size(),
  	       "Mode BER");
    chsnprintf(oledScreen[2].begin(), oledScreen[2].size(),
  	       "Source Interne");
    chsnprintf(oledScreen[3].begin(), oledScreen[3].size(),
  	       "P %d dbm", board.getTxPower());
  }
  
  void fillWhenError(void)
  {
    chsnprintf(oledScreen[0].begin(), oledScreen[0].size(),
  	       "ERREUR :");
    chsnprintf(oledScreen[1].begin(), oledScreen[1].size(),
  	       "%s", board.getError().c_str());
    chsnprintf(oledScreen[2].begin(), oledScreen[2].size(),
  	       "Mode = ");
    chsnprintf(oledScreen[3].begin(), oledScreen[3].size(),
  	       "%s", Ope::toAscii(board.getMode()));
  }
  

  void clearScreenBuffer(void)
  {
    for (auto& l : oledScreen) {
      l.clear();
    }
  }
  //
  //  chsnprintf(oledScreen[0].begin(), oledScreen[0].size(),
  //	       "hello word");

}
