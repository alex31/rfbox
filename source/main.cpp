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


void _init_chibios() __attribute__ ((constructor(101)));
void _init_chibios() {
  halInit();
  chSysInit();
  initHeap();
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
  const uint32_t baud = DIP::getDip(DIPSWITCH::BERBAUD) ? baudHigh : baudLow;
  board.setBaud(baud);
  
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
      if (baud == baudLow)
	opMode = rfMode == RfMode::RX ? Ope::Mode::RF_RX_EXTERNAL_OOK : Ope::Mode::RF_TX_EXTERNAL_OOK;
      else
	opMode = rfMode == RfMode::RX ? Ope::Mode::RF_RX_EXTERNAL_FSK : Ope::Mode::RF_TX_EXTERNAL_FSK;
    }
  }
  chDbgAssert(opMode != Ope::Mode::NONE, "internal logic error");
  board.setMode(opMode);
  DebugTrace("Ope::opMode = %u", static_cast<uint16_t>(opMode));
  
  Ope::Status opStatus = {};
  do {
    Radio::init();
    opStatus = Ope::setMode(opMode, frequencyCarrier,
			    amplificationLevelDb);
    if (opStatus != Ope::Status::OK) {
      DebugTrace("Ope::setMode error status = %u",
		 static_cast<uint16_t>(opStatus));
      board.setError(Ope::toAscii(opStatus));
    }
  } while (opStatus != Ope::Status::OK);
  board.clearError();
  if (opStatus != Ope::Status::OK) {
    while (true) {
#ifdef LINE_LED_HEARTBEAT
      palToggleLine(LINE_LED_HEARTBEAT);
#endif
      chThdSleepMilliseconds(100);
  }

  }
  


#ifdef NOSHELL
  wdgStart(&WDGD1, &wdgcfg);
#endif
  
  while (true) {
    chThdSleepMilliseconds(100);
#ifdef NOSHELL
    wdgReset(&WDGD1);
#endif
  }
}


