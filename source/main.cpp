#include <ch.h>
#include <hal.h>
#include <algorithm>

#include "stdutil.h"	
#include "ttyConsole.hpp"	
#include "dip.hpp"
#include "modeTest.hpp"
#include "radio.hpp"
#include "operations.hpp"
#include "hardwareConf.hpp"

int main (void)
{
  halInit();
  chSysInit();
  initHeap();		

  consoleInit();	
  consoleLaunch();

  RADIO::init();
  
  const RfMode rfMode = DIP::getDip(DIPSWITCH::RXTX) ? RfMode::RX : RfMode::TX;
  if (rfMode == RfMode::RX) 
    DebugTrace("mode RX");
  else
    DebugTrace("mode TX");

  const bool rfEnable = DIP::getDip(DIPSWITCH::RFENABLE);
  if (rfEnable) 
    DebugTrace("RF Enable");
  else
    DebugTrace("RF Disable");

  const uint32_t frequencyCarrier = DIP::getDip(DIPSWITCH::FREQ) ?
    carrierFrequencyHigh : carrierFrequencyLow;

  const int8_t amplificationLevelDb = DIP::getDip(DIPSWITCH::PWRLVL) ?
    ampLevelDbLow : ampLevelDbHigh;
  const bool berMode = DIP::getDip(DIPSWITCH::BER);
  const uint32_t baud = DIP::getDip(DIPSWITCH::BERBAUD) ? baudLow : baudHigh;
  
  DebugTrace("carrier frequency %lu @ %d Db level", frequencyCarrier, amplificationLevelDb);

  Ope::Mode opMode = Ope::Mode::NONE;
  if (not rfEnable) {
    opMode = rfMode == RfMode::RX ? Ope::Mode::NORF_RX : Ope::Mode::NORF_TX;
  } else { // rfEnable
    if (berMode) {
      opMode = rfMode == RfMode::RX ? Ope::Mode::RF_RX_INTERNAL : Ope::Mode::RF_TX_INTERNAL;
    } else { // not in ber mode : regular mode
      opMode = rfMode == RfMode::RX ? Ope::Mode::RF_RX_EXTERNAL : Ope::Mode::RF_TX_EXTERNAL;
    }
  }
  chDbgAssert(opMode != Ope::Mode::NONE, "internal logic error");
  DebugTrace("Ope::opMode = %u", static_cast<uint16_t>(opMode));
  Ope::setMode(opMode, frequencyCarrier, amplificationLevelDb);
  // main thread does nothing
  DIP::start();
  if (berMode) {
    DebugTrace("start BER mode @ %lu baud",  baud);
    ModeTest::start(rfMode, baud);
  }
  
  chThdSleep(TIME_INFINITE);
}


