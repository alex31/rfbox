#include <ch.h>
#include <hal.h>
#include "stdutil.h"	
#include "airSensor.hpp"	


namespace {
  volatile bool testMode = false;
  volatile float testWindSpeed = 0;
  
  void enterTestMode(void);
  void leaveTestMode(void);

  static PWMConfig pwmcfg = {
  .frequency = 80'000'000,    
  .period    = 40'000'000, 
  .callback  = NULL,		 
  .channels  = {
    {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
    {.mode = PWM_OUTPUT_ACTIVE_HIGH, .callback = NULL},
    {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
    {.mode = PWM_OUTPUT_DISABLED, .callback = NULL}
  },
  .cr2  = 0, 
  .bdtr = 0,
  .dier = 0  
};


  
}





static THD_WORKING_AREA(waWindTestMode, 304);	
[[noreturn]] static void  windTestMode (void *arg)	
{
  (void)arg;					
  chRegSetThreadName("windTestMode");
  palEnableLineEvent(LINE_BUTTON_WINDTEST, PAL_EVENT_MODE_BOTH_EDGES);
  while(true) {
    palWaitLineTimeout(LINE_BUTTON_WINDTEST, TIME_INFINITE);
    uint32_t nlevel = palReadLine(LINE_BUTTON_WINDTEST);
    uint32_t level;

    // anti bouncing
    do {
      chThdSleepMilliseconds(20);
      level = nlevel;
      nlevel = palReadLine(LINE_BUTTON_WINDTEST);
    } while (nlevel != level);
    
    if (level == PAL_LOW)
      enterTestMode();
    else
      leaveTestMode();
  }
}

void windTestStart()
{
  pwmStart(&PWMD2, &pwmcfg);
  chThdCreateStatic(waWindTestMode, sizeof(waWindTestMode), NORMALPRIO, &windTestMode, nullptr);
}

bool isInWindTestMode(void)
{
  return testMode;
}

float getTestWindSpeed(void)
{
  return testWindSpeed;
}


namespace {
  void enterTestMode(void)
  {
    static constexpr float radius = 0.05f;
    static constexpr float correctiveFactor = 5.0f;
    static constexpr float edgeByRotation = 8.0f;

    // angle is giving a position between 0 and 63, and we get this as a a speed in m/s
    //     const float pulseDurationSecond = RTC2US(STM32_SYSCLK, pt) / 1e6;
    //  rps = 1.0 / (edgeByRotation * pulseDurationSecond);
    //  currentSpeed = 2 * 3.14f * radius * correctiveFactor * rps;

    
    const float currentSpeed= getAngle() * 360.0 / 640.0;
    if (currentSpeed == 0) {
      //        pwmEnableChannel(&PWMD2, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD2, 5000));
      pwmDisableChannel(&PWMD2, 1);
    } else {
      const float rps = currentSpeed / (2 * 3.14f * radius * correctiveFactor);
      const float pulseDurationSecond = 1 / (rps * edgeByRotation);
      const float newFreq = 0.5f / pulseDurationSecond;
      const pwmcnt_t newPeriod = PWMD2.config->frequency / newFreq;
      pwmChangePeriod(&PWMD2, newPeriod);

      pwmEnableChannel(&PWMD2, 1, newPeriod / 2);
    }
    
    palSetLineMode(LINE_WIND_SPEED_OUT, PAL_MODE_ALTERNATE(WIND_SPEED_OUT_TIM_AF));
    testWindSpeed = currentSpeed;
    testMode = true;
  }

  
  void leaveTestMode(void)
  {
    pwmDisableChannel(&PWMD2, 1);
    palSetLineMode(LINE_WIND_SPEED_OUT, PAL_MODE_OUTPUT_PUSHPULL);
    testMode = false;
  }
}

