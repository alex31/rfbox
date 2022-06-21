#pragma once

#include <ch.h>
#include <hal.h>
#include <utility>

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
    encMode(mode) {  rccEnable();}
  std::pair<bool, uint16_t> getCnt(void);
  void start(void);
  void stop(void);
  void reset(void);
  bool zeroSetDone(void) {return topNordValue >= 0;}
private:
  void rccEnable(void) {rccEnableAPB1R1(RCC_APB1ENR1_LPTIM1EN, true)};
  bool cntIsUpdated(void);
  
  EncoderMode encMode;
  uint16_t lastCnt=0U;
  volatile int32_t topNordValue = -1;
};





