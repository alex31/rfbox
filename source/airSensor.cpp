#include <ch.h>
#include <hal.h>
#include <string>
#include "stdutil.h"	
#include "encoderTimer.hpp"	
#include "airSensor.hpp"	



namespace {
  uint8_t binaryToGray(uint8_t num);
  uint8_t grayToBinary(uint8_t num);

  constexpr float angleOffset = 146.0f;
  constexpr uint16_t cntOffset = angleOffset * 2048 / 360;
  volatile float currentSpeed = 0.0f;
  volatile uint32_t ledBlinkPeriod = 1000;
  EncoderModeLPTimer1 lptim1d;
  EncoderTIM1 tim1Cnt(EncoderMode::CH1_COUNTING);
}



static THD_WORKING_AREA(waTraceTim1, 304);	
[[noreturn]] static void  traceTim1 (void *arg)	
{
  (void)arg;					
  chRegSetThreadName("traceTim1Cnt");		
  tim1Cnt.start();

#ifdef NOSHELL
  chThdSleep(TIME_INFINITE);
#endif
  chThdSleep(TIME_INFINITE);
  while (true) {				
    auto [u, cnt] = tim1Cnt.getCnt();
    if (u) {
      DebugTrace("tim1 CNT = %u", cnt);
    }
    chThdSleepMilliseconds(1);		
  }
}

static THD_WORKING_AREA(waWindSpeed, 304);	
[[noreturn]] static void  windSpeed (void *arg)	
{
  (void)arg;					
  chRegSetThreadName("windSpeedCnt");
  /*
    diametre : 5cm,
    rendement :  50%
    8 fronts par tour
    distance entre deux fronts en mètres : (0.05 * 0.5) / 8
    4563042
  */
  
  while(true) {
    static constexpr float radius = 0.05f;
    static constexpr float correctiveFactor = 5.0f;
    static constexpr float edgeByRotation = 8.0f;

    const rtcnt_t pt =  tim1Cnt.getPulseTime();
    if (pt != 0)  {
      const float pulseDurationSecond = RTC2US(STM32_SYSCLK, pt) / 1e6;
      const float rps = 1.0 / (edgeByRotation * pulseDurationSecond);
      currentSpeed = 2 * 3.14f * radius * correctiveFactor * rps;
    } else {
      currentSpeed = 0;
    }

    
    DebugTrace("windSpeed = [%lu] {%2.f} %.2f m/s",
	       pt, rps, currentSpeed);
    chThdSleepMilliseconds(100);
  }
}


static THD_WORKING_AREA(waTraceLptim1, 304);	
[[noreturn]] static void  traceLptim1 (void *arg)	
{
  (void)arg;					
  chRegSetThreadName("traceLptim1");		
  lptim1d.start();
  while (not lptim1d.zeroSetDone()) {
    chThdSleepMilliseconds(100);
    palToggleLine(LINE_NOT_ZEROED);
  }

  palClearLine(LINE_NOT_ZEROED);

  auto cb =  [] (void *) {
    static uint8_t lastCode = 0xFF;
    auto [_, cnt] = lptim1d.getCnt();
    cnt = (cnt + cntOffset) % 2048;
    const uint8_t angle = cnt >> 5;
    const uint8_t grayCode = binaryToGray(angle);
    if (lastCode != grayCode) {
      lastCode = grayCode;
      uint8_t portGrayCode = grayCode << 2;
      // dispatch 6 bits code on Port A : 1,3,4,5,6,7
      (portGrayCode |= ((portGrayCode & 0b100) >> 1)) &= 0b11111010;
      // atomic 6 bits gray code update
      palWriteGroup(GPIOA, 0b11111010, 0, portGrayCode);
    }
  };

  palEnableLineEvent(LINE_WIND_DIR_CHA_IT, PAL_EVENT_MODE_FALLING_EDGE);
  palSetLineCallback(LINE_WIND_DIR_CHA_IT, cb, NULL);
  
#ifdef NOSHELL
  chThdSleep(TIME_INFINITE);
#endif

  uint8_t lastCode = 0xFF;
  while (true) {				
    auto [_, cnt] = lptim1d.getCnt();
    cnt = (cnt + cntOffset) % 2048;
    const uint8_t angle = cnt >> 5;
    const uint8_t grayCode = binaryToGray(angle);
    if (lastCode != grayCode) {
      lastCode = grayCode;
      uint8_t portGrayCode = grayCode << 2;
      // dispatch 6 bits code on Port A : 1,3,4,5,6,7
      (portGrayCode |= ((portGrayCode & 0b100) >> 1)) &= 0b11111010;
      // atomic 6 bits gray code update
      std::string binGray(binary_fmt(grayCode, 8));
      std::string binPort(binary_fmt(portGrayCode, 8));
      DebugTrace("lptim1 CNT = %u [%u] [g:0x%x -> 0x%x] {%s -> %s}",
		 cnt, angle, grayCode, portGrayCode,
		 binGray.c_str(), binPort.c_str());
    }
    chThdSleepMilliseconds(100);
  }
}



void airSensorStart (void)
{
  chThdCreateStatic(waTraceTim1, sizeof(waTraceTim1), NORMALPRIO, &traceTim1, NULL);
  chThdCreateStatic(waWindSpeed, sizeof(waWindSpeed), NORMALPRIO, &windSpeed, NULL);
  chThdCreateStatic(waTraceLptim1, sizeof(waTraceLptim1), NORMALPRIO - 1, &traceLptim1, NULL);

  palEnableLineEvent(LINE_ENCODER_ZERO, PAL_EVENT_MODE_FALLING_EDGE);
  palSetLineCallback(LINE_ENCODER_ZERO,
		     [] (void *) {
		       lptim1d.reset();
		     },
		     NULL);
}


uint8_t getAngle (void)
{
  auto [_, cnt] = lptim1d.getCnt();
  cnt = (cnt + cntOffset) % 2048;
  const uint8_t angle = cnt >> 5;
  return angle;
}

float getWindSpeed (void)
{
  return currentSpeed;
}


bool airDirectionIsCalibrated(void)
{
  return lptim1d.zeroSetDone();
}

namespace {
  uint8_t binaryToGray(uint8_t num)
  {
    return num ^ (num >> 1); // The operator >> is shift right. The operator ^ is exclusive or.
  }
  
  // This function converts a reflected binary Gray code number to a binary number.
  [[maybe_unused]] uint8_t grayToBinary(uint8_t num)
  {
    uint8_t mask = num;
    while (mask) {           // Each Gray code bit is exclusive-ored with all more significant bits.
      mask >>= 1;
      num ^= mask;
    }
    return num;
  }
}
