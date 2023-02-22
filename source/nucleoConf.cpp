#include <ch.h>
#include <hal.h>
#include "stdutil.h"
#include "nucleoConf.hpp"
#include "bboard.hpp"

/*
VCP Unplug
LINE_EXTVCP_RX level in PULLDOWN = 0
LINE_RADIO_SCK level in PULLDOWN = 0
LINE_EXTVCP_RX level in PULLUP = 1
LINE_RADIO_SCK level in PULLUP = 1

VCP Plug idle
LINE_EXTVCP_RX level in PULLDOWN = 1
LINE_RADIO_SCK level in PULLDOWN = 0
LINE_EXTVCP_RX level in PULLUP = 1
LINE_RADIO_SCK level in PULLUP = 1
*/

namespace {
  inline void w() {chThdSleepMicroseconds(10);}
}

bool NucleoConf::checkShort()
{
  bool sb16Short = false;
  bool sb18Short = false;
  
  palSetLineMode(LINE_RADIO_MISO, PAL_MODE_INPUT_PULLUP);
  palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_OUTPUT_PUSHPULL);
  palClearLine(LINE_EXTVCP_TX); w();

  sb16Short = sb16Short || (palReadLine(LINE_RADIO_MISO) == PAL_LOW);

  palSetLineMode(LINE_RADIO_MISO, PAL_MODE_INPUT_PULLDOWN);
  palSetLine(LINE_EXTVCP_TX); w();

  sb16Short = sb16Short || (palReadLine(LINE_RADIO_MISO) == PAL_HIGH);

  if (sb16Short) {
    board.setError("SB16 Short");
    chThdSleepSeconds(1);
  }
  
  palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_INPUT_PULLDOWN);
  palSetLineMode(LINE_RADIO_SCK, PAL_MODE_INPUT); w();
  const auto rxPdLev = palReadLine(LINE_EXTVCP_RX);
  palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_INPUT_PULLUP); w();
  const auto rxPuLev = palReadLine(LINE_EXTVCP_RX);

  if ((rxPdLev == PAL_LOW) and (rxPuLev == PAL_HIGH)) {
    // USB serial is unplug and we can test for short
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(LINE_RADIO_SCK, PAL_MODE_INPUT_PULLUP);
    palClearLine(LINE_EXTVCP_TX); w();
    sb18Short = sb18Short || (palReadLine(LINE_RADIO_SCK) == PAL_LOW);
    palSetLineMode(LINE_RADIO_SCK, PAL_MODE_INPUT_PULLDOWN);
    palSetLine(LINE_EXTVCP_TX); w();
    sb18Short = sb18Short || (palReadLine(LINE_RADIO_SCK) == PAL_HIGH);
  } else {
    DebugTrace("USB serial plugged, sd18 test for short skipped");
  }

  if (sb18Short) {
    board.setError("SB18 Short");
  }

  palSetLineMode(LINE_RADIO_MISO, PAL_MODE_ALTERNATE(AF_LINE_RADIO_MISO));
  palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
  palSetLineMode(LINE_RADIO_SCK, PAL_MODE_ALTERNATE(AF_LINE_RADIO_SCK));
  palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_RX));

  chThdSleepMicroseconds(10);
  
  return sb16Short || sb18Short;
}


 void NucleoConf::examineSb18()
 {
  palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_INPUT_PULLDOWN);
  palSetLineMode(LINE_RADIO_SCK, PAL_MODE_INPUT_PULLDOWN);
  chThdSleepMicroseconds(10);
  
  DebugTrace("LINE_EXTVCP_RX level in PULLDOWN = %ld",
	     palReadLine(LINE_EXTVCP_RX));
  DebugTrace("LINE_RADIO_SCK level in PULLDOWN = %ld",
	     palReadLine(LINE_RADIO_SCK));

  palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_INPUT_PULLUP);
  palSetLineMode(LINE_RADIO_SCK, PAL_MODE_INPUT_PULLUP);
  chThdSleepMicroseconds(10);
  
  DebugTrace("LINE_EXTVCP_RX level in PULLUP = %ld",
	     palReadLine(LINE_EXTVCP_RX));
  DebugTrace("LINE_RADIO_SCK level in PULLUP = %ld",
	     palReadLine(LINE_RADIO_SCK));
 

  
  palSetLineMode(LINE_RADIO_SCK, PAL_MODE_ALTERNATE(AF_LINE_RADIO_SCK));
  palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_RX));
  chThdSleepMicroseconds(10);
 }
