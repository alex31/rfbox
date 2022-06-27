#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "encoderTimer.hpp"	
#include "airSensor.hpp"	
#include "oledDisplay.hpp"
#include "ssd1306.h"


namespace {
  void displayAirData(void);
  constexpr uint32_t  I2C_FAST_400KHZ_DNF3_R30NS_F30NS_PCLK80MHZ_TIMINGR = 0x00A02689;
  uint32_t stm32_cr1_dnf(uint8_t n)  {return ((n & 0x0f) << 8);}
  static const I2CConfig i2ccfg_400 = {
    .timingr = I2C_FAST_400KHZ_DNF3_R30NS_F30NS_PCLK80MHZ_TIMINGR, // Refer to the STM32L4 reference manual
    .cr1 =  stm32_cr1_dnf(3U), // Digital noise filter disabled (timingr should be aware of that)
    .cr2 = 0 // Only the ADD10 bit can eventually be specified here (10-bit addressing mode)
  } ;

  const GFXfont font = Lato_Heavy_18;

  THD_WORKING_AREA(waOledDisplay, 1024);	
  [[noreturn]] static void  oledDisplay (void *arg);	
 
}



void oledStart(void)
{
  chThdCreateStatic(waOledDisplay, sizeof(waOledDisplay), NORMALPRIO, &oledDisplay, NULL);
}


namespace {
  [[noreturn]] static void  oledDisplay (void *)	
  {
    chRegSetThreadName("oledDisplay");		
    
    i2cStart(&I2CD1, &i2ccfg_400);
    ssd1306_Init();
    
    do {
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
      chThdSleepMilliseconds(500);
    } while (not airDirectionIsCalibrated());
    
    while (true) {
      displayAirData();
      chThdSleepMilliseconds(100);
    }
  }
  
  
  void displayAirData(void) 
  {
    static int16_t hearbeatPos = 10;
    static int16_t hearbeatInc = 5;
    static uint32_t count = 0;

    if (++count == 5) {
      count = 0;
      ssd1306_Fill(BLACK);
      ssd1306_MoveCursor(0, 16);
      ssd1306_WriteFmt(font, WHITE, "S= %04.1f m/s", getWindSpeed());
      ssd1306_MoveCursor(0, 40);
      ssd1306_WriteFmt(font, WHITE, "D= %03u deg", getAngle() * 360U / 64U);
    }
    ssd1306_DrawLine(0, 60, 127, 60, BLACK);
    ssd1306_DrawLine(hearbeatPos, 60, hearbeatPos + 20, 60, WHITE);
    ssd1306_UpdateScreen();
    if ((hearbeatPos > 100) or (hearbeatPos < 10)) {
      hearbeatInc = -hearbeatInc;
    }
    hearbeatPos += hearbeatInc;
  }
}
