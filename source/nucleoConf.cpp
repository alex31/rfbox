#include <ch.h>
#include <hal.h>
#include "nucleoConf.hpp"
#include "bboard.hpp"


bool NucleoConf::checkShort()
{
  bool sb16Short = false;
  // bool sb18Short = true;
  
  palSetLineMode(LINE_RADIO_MISO, PAL_MODE_INPUT_PULLUP);
  palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_OUTPUT_PUSHPULL);
  palClearLine(LINE_EXTVCP_TX);
  chThdSleepMicroseconds(10);
  sb16Short = sb16Short || (palReadLine(LINE_RADIO_MISO) == PAL_LOW);

  palSetLineMode(LINE_RADIO_MISO, PAL_MODE_INPUT_PULLDOWN);
  palSetLine(LINE_EXTVCP_TX);
  chThdSleepMicroseconds(10);
  sb16Short = sb16Short || (palReadLine(LINE_RADIO_MISO) == PAL_HIGH);

  if (sb16Short) {
    board.setError("SB16 Short");
  }
  
  // palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_INPUT_PULLDOWN);
  // palSetLineMode(LINE_RADIO_SCK, PAL_MODE_INPUT_PULLUP);
  // for (int i=0; i<200; i++) {
  //   chThdSleepMicroseconds(100);
  //   const bool same =  palReadLine(LINE_RADIO_SCK) == palReadLine(LINE_EXTVCP_RX);
  //   sb18Short = sb18Short && same;
  // }

  // palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_INPUT_PULLUP);
  // palSetLineMode(LINE_RADIO_SCK, PAL_MODE_INPUT_PULLDOWN);
  // for (int i=0; i<200; i++) {
  //   chThdSleepMicroseconds(100);
  //   const bool same =  palReadLine(LINE_RADIO_SCK) == palReadLine(LINE_EXTVCP_RX);
  //   sb18Short = sb18Short && same;
  // }

  // if (sb18Short) {
  //   board.setError("SB18 Short");
  // }


  palSetLineMode(LINE_RADIO_MISO, PAL_MODE_ALTERNATE(AF_LINE_RADIO_MISO));
  palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
  // palSetLineMode(LINE_RADIO_SCK, PAL_MODE_ALTERNATE(AF_LINE_RADIO_SCK));
  // palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_RX));

  chThdSleepMicroseconds(10);
  
  return sb16Short; // || sb18Short
}
