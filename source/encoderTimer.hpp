#pragma once

#include <ch.h>
#include <hal.h>
#include <utility>
#include "lpTimerProxy.h"

enum class EncoderMode {QUADRATURE, CH1_COUNTING};


class EncoderModeTimer {
public:
  EncoderModeTimer(stm32_tim_t *_timer, EncoderMode mode = EncoderMode::QUADRATURE) :
    timer(_timer), encMode(mode) {start();}
  std::pair<bool, uint16_t> getCnt(void) {
    return {cntIsUpdated(), timer->CNT};
  }
private:
  void start(void);
  void rccEnable(void);
  bool cntIsUpdated(void);
  
  stm32_tim_t * const timer;
  EncoderMode encMode;
  uint16_t lastCnt=0U;
};

class EncoderModeLPTimer1 {
public:
  EncoderModeLPTimer1(EncoderMode mode = EncoderMode::QUADRATURE) :
    encMode(mode) {}
  std::pair<bool, uint16_t> getCnt(void) {
    return {cntIsUpdated(), LPTIM1->CNT};
  }
  void start(void);
private:
  void rccEnable(void) {rccEnableAPB1R1(RCC_APB1ENR1_LPTIM1EN, true)};
  bool cntIsUpdated(void);
  
  EncoderMode encMode;
  uint16_t lastCnt=0U;
};





