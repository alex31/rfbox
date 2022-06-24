#include "encoderTimer.hpp"
#include <algorithm>
#include  <utility>

volatile rtcnt_t EncoderTIM1::diff_ts = 0;

void EncoderTIM1::start(void)
{
  rccEnable();

  if (encMode == EncoderMode::QUADRATURE) {
    TIM1->PSC = 0;	    // prescaler must be set to zero
    TIM1->SMCR = 1;          // Encoder mode 1 : count on TI1 only
    TIM1->CCER = 0;          // rising edge polarity
    TIM1->ARR = 0xFFFF;      // count from 0-ARR or ARR-0
    TIM1->CCMR1 = 0xC1C1;    // f_DTS/16, N=8, IC1->TI1, IC2->TI2
    TIM1->CNT = 0;           // Initialize counter
    TIM1->EGR = 1;           // generate an update event
    TIM1->CR1 = 1;           // Enable the counter
  } else { // UP_COUNTING ON CHANNEL 1
    TIM1->PSC = 0;	      // prescaler must be set to zero
    TIM1->SMCR = 0x57;       // count pulses on CH1
    TIM1->CCER = 0;          // rising edge polarity
    TIM1->ARR = 1024 / 8;    // generate 8 edge by rotation
    TIM1->CCMR1 = 0xC1;      // f_DTS/16, N=8, CC1S=01
    TIM1->CNT = 0;           // Initialize counter
    TIM1->EGR = 1;           // generate an update event
    TIM1->DIER = STM32_TIM_DIER_UIE; // update generate IRQ
    TIM1->CR1 = 1;           // Enable the counter
  }
}

bool EncoderTIM1::cntIsUpdated(void)
{
  const uint16_t newV = TIM1->CNT;
  const bool change = (newV != lastCnt);
  lastCnt = newV;
  return change;
}

void EncoderTIM1::rccEnable(void)
{
  rccEnableTIM1(true);
  rccResetTIM1();
  nvicEnableVector(STM32_TIM1_UP_TIM16_NUMBER, 1);
};

// when update is done after CNT reach ARR, this ISR is called
// poor man frequency diviser ...
CH_FAST_IRQ_HANDLER(STM32_TIM1_UP_TIM16_HANDLER) { 
  static rtcnt_t last_ts = 0;
  uint32_t sr = TIM1->SR;
  sr &= (TIM1->DIER & STM32_TIM_DIER_IRQ_MASK);
  TIM1->SR = ~sr;
  palToggleLine(LINE_WIND_SPEED_OUT);
  const rtcnt_t now = chSysGetRealtimeCounterX();
  EncoderTIM1::diff_ts = now - last_ts;
  last_ts = now;
}

void EncoderModeLPTimer1::stop(void)
{
  LPTIM1->CR = 0;
}

void EncoderModeLPTimer1::reset(void)
{
  topNordValue = LPTIM1->CNT;
}

void EncoderModeLPTimer1::start(void)
{
  chDbgAssert(encMode == EncoderMode::QUADRATURE, "only quadrature encoder mode is possible");
  LPTIM1->CFGR = (0b00 << LPTIM_CFGR_CKPOL_Pos);
  LPTIM1->CFGR |= LPTIM_CFGR_ENC;
  LPTIM1->CR = LPTIM_CR_ENABLE;
  LPTIM1->ICR |= LPTIM_ICR_ARROKCF;
  LPTIM1->ARR = 0xFFFF;
  while(!(LPTIM1->ISR & LPTIM_ISR_ARROK)) {};

  LPTIM1->CR |= LPTIM_CR_CNTSTRT;
}

bool EncoderModeLPTimer1::cntIsUpdated(void)
{
  uint16_t fv = LPTIM1->CNT;
  while (fv != LPTIM1->CNT) {fv = LPTIM1->CNT;}
  
  const bool change = (fv != lastCnt);
  lastCnt = fv;
  return change;
}

std::pair<bool, uint16_t> EncoderModeLPTimer1::getCnt(void)
{
  uint16_t fv = LPTIM1->CNT;
  while (fv != LPTIM1->CNT) {fv = LPTIM1->CNT;}

  if (topNordValue < 0) {
    return {false, 0};
  } else {
    const int32_t ifv = fv;
    const uint16_t counter = topNordValue >= ifv ?
      topNordValue - ifv :
      2048U - std::clamp(ifv - topNordValue, 0L, 2048L);
    
    return {cntIsUpdated(), counter};
  }
}
