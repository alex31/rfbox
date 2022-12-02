#include <ch.h>
#include <hal.h>
#include <algorithm>
#include "stdutil.h"
#ifndef NOSHELL
#include "ttyConsole.hpp"
#endif
#include "dip.hpp"
#include "radio.hpp"
#include "operations.hpp"
#include "hardwareConf.hpp"
#include "oledDisplay.hpp"
#include "bboard.hpp"
#include "crcv1.h"


void _init_chibios() __attribute__ ((constructor(101)));
void _init_chibios() {
  halInit();
  chSysInit();
  initHeap();
  crcInit();
}


#ifdef NOSHELL
// LSI @ 32Khz, watchdog after 1s without watchdog reset
static const WDGConfig wdgcfg = {
  .pr           = STM32_IWDG_PR_32,
  .rlr          = STM32_IWDG_RL(1000),
#if STM32_IWDG_IS_WINDOWED
  .winr         = STM32_IWDG_WIN_DISABLED,
#endif
};
#endif

int main (void)
{
#ifndef NOSHELL
  consoleInit();	
  consoleLaunch();
#endif
  
  Oled::start();
  const RfMode rfMode = DIP::getDip(DIPSWITCH::RXTX) ? RfMode::TX : RfMode::RX;
  if (rfMode == RfMode::RX) {
    DebugTrace("mode RX");
  } else {
    DebugTrace("mode TX");
  }
  DIP::start();
  const bool rfEnable = DIP::getDip(DIPSWITCH::RFENABLE);
  board.setRfEnable(rfEnable);
  if (rfEnable) {
    DebugTrace("RF Enable");
  } else {
    DebugTrace("RF Disable");
  }
  const uint32_t frequencyCarrier = DIP::getDip(DIPSWITCH::FREQ) ?
    carrierFrequencyHigh : carrierFrequencyLow;

  const int8_t amplificationLevelDb = DIP::getDip(DIPSWITCH::PWRLVL) ?
    ampLevelDbHigh : ampLevelDbLow;
  const bool berMode = DIP::getDip(DIPSWITCH::BER);
  const  BitRateIndex bitRateIndex = DIP::getDip(DIPSWITCH::BAUD_MODUL) ?
    BitRateIndex::High :
    BitRateIndex::Low;
  board.setBitRateIdx(bitRateIndex);
  
  DebugTrace("carrier frequency %lu @ %d Db level", frequencyCarrier, amplificationLevelDb);
  board.setFreq(frequencyCarrier);
  board.setTxPower(amplificationLevelDb);
  Ope::Mode opMode = Ope::Mode::NONE;
  if (not rfEnable) {
    if (berMode) {
      // forbidden combination
      board.setError("BER No RF invalid");
      chThdSleep(TIME_INFINITE);
    } else {
      opMode = rfMode == RfMode::RX ? Ope::Mode::NORF_RX : Ope::Mode::NORF_TX;
    }
  } else { // rfEnable
    if (berMode) {
      opMode = rfMode == RfMode::RX ? Ope::Mode::RF_RX_INTERNAL : Ope::Mode::RF_TX_INTERNAL;
    } else { // not in ber mode : regular mode
      if (bitRateIndex == BitRateIndex::Low)
	opMode = rfMode == RfMode::RX ? Ope::Mode::RF_RX_EXTERNAL_OOK : Ope::Mode::RF_TX_EXTERNAL_OOK;
      else
	opMode = rfMode == RfMode::RX ? Ope::Mode::RF_RX_EXTERNAL_FSK : Ope::Mode::RF_TX_EXTERNAL_FSK;
    }
  }
  chDbgAssert(opMode != Ope::Mode::NONE, "internal logic error");
  board.setMode(opMode);
  DebugTrace("Ope::opMode = %u", static_cast<uint16_t>(opMode));
  
  Ope::Status opStatus = {};
  Ope::ajustParamIfFsk(opMode);
  do {
    const Rfm69Status rstatus = Radio::init(opMode);
    if (rstatus == Rfm69Status::INIT_ERROR) {
      board.setError("RF FAIL");
      chThdSleep(TIME_INFINITE);
    }
    opStatus = Ope::setMode(opMode);
    if (opStatus != Ope::Status::OK) {
      DebugTrace("Ope::setMode error status = %u [%s]",
		 static_cast<uint16_t>(opStatus), Ope::toAscii(opStatus));
      board.setError(Ope::toAscii(opStatus));
    }
  } while (opStatus != Ope::Status::OK);
  board.clearError();

#ifdef NOSHELL
  wdgStart(&WDGD1, &wdgcfg);
#endif
  
#ifdef NOSHELL
  while (true) {
    chThdSleepMilliseconds(100);
    wdgReset(&WDGD1);
  }
#else
  chThdSleep(TIME_INFINITE);
#endif
}


