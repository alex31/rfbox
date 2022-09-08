#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "ssd1306.h"


namespace {
  const GFXfont font = Lato_Heavy_18;

  THD_WORKING_AREA(waOledDisplay, 1024);	
  [[noreturn]] static void  oledDisplay (void *arg);	
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
      ssd1306_MoveCursor(0, 25);
      ssd1306_Fill(BLACK);
      ssd1306_WriteFmt(font, WHITE, "SCREEN DEMO");
      ssd1306_UpdateScreen();
      chThdSleepMilliseconds(500);
    } 
  }
  
  
}
