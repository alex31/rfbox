#pragma once

#include <ch.h>
#include <hal.h>
#include <utility>

enum class EncoderMode {QUADRATURE, CH1_COUNTING};


class EncoderTIM1 {
public:
  static volatile rtcnt_t diff_ts;
  EncoderTIM1(EncoderMode mode = EncoderMode::QUADRATURE) :
    encMode(mode) {}
  std::pair<bool, uint16_t> getCnt(void) {
    return {cntIsUpdated(), TIM1->CNT};
  }
  void start(void);
  rtcnt_t getPulseTime(void) {return diff_ts;}
private:
  void rccEnable(void);
  bool cntIsUpdated(void);
  
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





