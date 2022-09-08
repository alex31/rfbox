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
#include "dio2Spy.hpp"
#include "oledDisplay.hpp"
#ifdef STM32F4xx_MCUCONF
#include "notGate.hpp"
#endif

int main (void)
{
  halInit();
  chSysInit();
  initHeap();		

  consoleInit();	
  consoleLaunch();
  Oled::start();
  const RfMode rfMode = DIP::getDip(DIPSWITCH::RXTX) ? RfMode::TX : RfMode::RX;
  if (rfMode == RfMode::RX) {
    DebugTrace("mode RX");
  } else {
    DebugTrace("mode TX");
  }
  DIP::start();
  const bool rfEnable = DIP::getDip(DIPSWITCH::RFENABLE);
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
  
  Ope::Status opStatus = {};
  do {
    Radio::init();
    opStatus = Ope::setMode(opMode, frequencyCarrier,
			    amplificationLevelDb);
    if (opStatus != Ope::Status::OK) {
      DebugTrace("Ope::setMode error status = %u",
		 static_cast<uint16_t>(opStatus));
    }
  } while (opStatus != Ope::Status::OK);

  if (opStatus != Ope::Status::OK) {
    while (true) {
#ifdef LINE_LED_HEARTBEAT
      palToggleLine(LINE_LED_HEARTBEAT);
#endif
      chThdSleepMilliseconds(100);
  }

  }
  
  if (berMode) {
    DebugTrace("start BER mode @ %lu baud",  baud);
    ModeTest::start(rfMode, baud);
  }

  Dio2Spy::start(LINE_EXTVCP_RX);
#ifdef STM32F4xx_MCUCONF
  notGateStart(PWMD1);
#endif
  
#ifdef LINE_LED_HEARTBEAT
  while (true) {
    palToggleLine(LINE_LED_HEARTBEAT);
    chThdSleepMilliseconds(500);
  }
#else
  chThdSleep(TIME_INFINITE);
#endif
}


