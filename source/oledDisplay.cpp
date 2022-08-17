#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "oledDisplay.hpp"
#include "ssd1306.h"
#include <utility>


namespace {
  void displayAirData(void);
  constexpr uint32_t  I2C_FAST_400KHZ_DNF3_R30NS_F30NS_PCLK80MHZ_TIMINGR = 0x00A02689;
  constexpr uint32_t stm32_cr1_dnf(uint8_t n)  {return ((n & 0x0f) << 8);}
  static constexpr I2CConfig i2ccfg_400 = {
    .timingr = I2C_FAST_400KHZ_DNF3_R30NS_F30NS_PCLK80MHZ_TIMINGR, // Refer to the STM32L4 reference manual
    .cr1 =  stm32_cr1_dnf(3U), // Digital noise filter disabled (timingr should be aware of that)
    .cr2 = 0 // Only the ADD10 bit can eventually be specified here (10-bit addressing mode)
  } ;

  const GFXfont font = Lato_Heavy_14;

  THD_WORKING_AREA(waOledDisplay, 1024);	
  [[noreturn]] static void  oledDisplay (void *arg);	
}


void oledStart(void)
{
  chThdCreateStatic(waOledDisplay, sizeof(waOledDisplay), NORMALPRIO, &oledDisplay, nullptr);
}


namespace {
  [[noreturn]] static void  oledDisplay (void *)	
  {
    chRegSetThreadName("oledDisplay");		
    
    i2cStart(&SSD1306_I2CD, &i2ccfg_400);
    ssd1306_Init();
    
    ssd1306_MoveCursor(0, 25);
    ssd1306_Fill(BLACK);
    ssd1306_WriteFmt(font, WHITE, "CALIBRATE");
    ssd1306_UpdateScreen();
    chThdSleepMilliseconds(500);
    
    ssd1306_MoveCursor(0, 16);
    ssd1306_Fill(WHITE);
    ssd1306_WriteFmt(font, BLACK, "Rotate");
    ssd1306_MoveCursor(0, 40);
    ssd1306_WriteFmt(font, BLACK, "Arrow");
    ssd1306_UpdateScreen();
  
    while (true) {
      displayAirData();
      chThdSleepMilliseconds(20);
    }
  }
  
  
  void displayAirData(void) 
  {
    static int16_t hearbeatPos = 10;
    static int16_t hearbeatInc = 2;
    static uint32_t count = 0;
    const auto [bg, fg] = std::pair(BLACK, WHITE);
    
    if (++count == 10) {
      count = 0;
      ssd1306_Fill(bg);
      ssd1306_MoveCursor(0, 16);
      ssd1306_WriteFmt(font, fg, "S= %04.1f m/s", 42.24);
      ssd1306_MoveCursor(0, 40);
      ssd1306_WriteFmt(font, fg, "D= %03u deg", 123);
    }
    ssd1306_DrawLine(0, 60, 127, 60, bg);
    ssd1306_DrawLine(hearbeatPos, 60, hearbeatPos + 20, 60, fg);
    ssd1306_UpdateScreen();
    if ((hearbeatPos > 100) or (hearbeatPos < 10)) {
      hearbeatInc = -hearbeatInc;
    }
    hearbeatPos += hearbeatInc;
  }
}