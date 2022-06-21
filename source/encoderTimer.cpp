#include "encoderTimer.hpp"
#include <algorithm>
void EncoderModeTimer::start(void)
{
  rccEnable();

  if (encMode == EncoderMode::QUADRATURE) {
    timer->PSC = 0;	    // prescaler must be set to zero
    timer->SMCR = 1;          // Encoder mode 1 : count on TI1 only
    timer->CCER = 0;          // rising edge polarity
    timer->ARR = 0xFFFF;      // count from 0-ARR or ARR-0
    timer->CCMR1 = 0xC1C1;    // f_DTS/16, N=8, IC1->TI1, IC2->TI2
    timer->CNT = 0;           // Initialize counter
    timer->EGR = 1;           // generate an update event
    timer->CR1 = 1;           // Enable the counter
  } else { // UP_COUNTING ON CHANNEL 1
    timer->PSC = 0;	    // prescaler must be set to zero
    timer->SMCR = 1;          // Encoder mode 1 : count on TI1 only
    timer->CCER = 0;          // rising edge polarity
    timer->ARR = 0xFFFF;      // count from 0-ARR or ARR-0
    timer->CCMR1 = 0xC1C1;    // f_DTS/16, N=8, IC1->TI1, IC2->TI2
    timer->CNT = 0;           // Initialize counter
    timer->EGR = 1;           // generate an update event
    timer->CR1 = 1;           // Enable the counter
  }
}

bool EncoderModeTimer::cntIsUpdated(void)
{
  const uint16_t newV = timer->CNT;
  const bool change = (newV != lastCnt);
  lastCnt = newV;
  return change;
}
void EncoderModeTimer::rccEnable(void)
{
#ifdef TIM2
  if (timer == STM32_TIM1) {
    rccEnableTIM1(true);
    rccResetTIM1();
  }
#endif
#ifdef TIM2
  else  if (timer == STM32_TIM2) {
    rccEnableTIM2(true);
    rccResetTIM2();
  }
#endif
#ifdef TIM3
  else  if (timer == STM32_TIM3) {
    rccEnableTIM3(true);
    rccResetTIM3();
  }
#endif
#ifdef TIM4
  else  if (timer == STM32_TIM4) {
    rccEnableTIM4(true);
    rccResetTIM4();
  }
#endif
#ifdef TIM5
  else  if (timer == STM32_TIM5) {
    rccEnableTIM5(true);
    rccResetTIM5();
  }
#endif
#ifdef TIM8
  else  if (timer == STM32_TIM8) {
    rccEnableTIM8(true);
    rccResetTIM8();
  }
#endif
#ifdef TIM9
  else  if (timer == STM32_TIM9) {
    rccEnableTIM9(true);
    rccResetTIM9();
  }
#endif
#ifdef TIM10
  else  if (timer == STM32_TIM10) {
    rccEnableTIM10(true);
    rccResetTIM10();
  }
#endif
#ifdef TIM11
  else  if (timer == STM32_TIM11) {
    rccEnableTIM11(true);
    rccResetTIM11();
  }
#endif
#ifdef TIM12
  else  if (timer == STM32_TIM12) {
    rccEnableTIM12(true);
    rccResetTIM12();
  }
#endif
#ifdef TIM13
  else  if (timer == STM32_TIM13) {
    rccEnableTIM13(true);
    rccResetTIM13();
  }
#endif
#ifdef TIM14
  else  if (timer == STM32_TIM14) {
    rccEnableTIM14(true);
    rccResetTIM14();
  }
#endif
#ifdef TIM15
  else  if (timer == STM32_TIM15) {
    rccEnableTIM15(true);
    rccResetTIM15();
  }
#endif
#ifdef TIM16
  else  if (timer == STM32_TIM16) {
    rccEnableTIM16(true);
    rccResetTIM16();
  }
#endif
#ifdef TIM17
  else  if (timer == STM32_TIM17) {
    rccEnableTIM17(true);
    rccResetTIM17();
  }
#endif
#ifdef TIM18
  else  if (timer == STM32_TIM18) {
    rccEnableTIM18(true);
    rccResetTIM18();
  }
#endif
#ifdef TIM19
  else  if (timer == STM32_TIM19) {
    rccEnableTIM19(true);
    rccResetTIM19();
  }
#endif
  else {
    chSysHalt("not a valid timer");
  }
};


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
