#include "ch.h"
#include "hal.h"
#include "operations.hpp"
#include "stdutil.h"
#include "modeTest.hpp"
#include "modeExternal.hpp"
#include "modeFsk.hpp"
#include "radio.hpp"
#include "buffer.hpp"
#include "hardwareConf.hpp"
#include "dio2Spy.hpp"
#include "bboard.hpp"

namespace {
  enum class ElectricalStatus {FREE, HOLD};

  void buffer_NORF_TX();
  void buffer_NORF_RX();
  void buffer_RF_CALIBRATE_RSSI();
  void buffer_RF_RX_EXTERNAL_OOK();
  void buffer_RF_TX_EXTERNAL_OOK();
  void buffer_RF_RX_EXTERNAL_FSK();
  void buffer_RF_TX_EXTERNAL_FSK();
  void buffer_RF_RX_INTERNAL();
  void buffer_RF_TX_INTERNAL();

  void startRxTxEcho(void);
  void stopRxTxEcho(void);
}

namespace Ope {
  Status setMode(Mode opMode) {
    Status status = Status::OK;
    RfMode rfMode = RfMode::SLEEP;
    Buffer::setMode(Buffer::Mode::HiZ);
    stopRxTxEcho();
 
    switch (opMode) {
    case Mode::NONE:
      return Status::INTERNAL_ERROR;
      break ;

    case Mode::NORF_TX:
      buffer_NORF_TX();
      break ;
      
    case Mode::NORF_RX:
      buffer_NORF_RX();
      startRxTxEcho();
      break ;
      
    case Mode::RF_CALIBRATE_RSSI:
      rfMode = RfMode::RX;
      buffer_RF_CALIBRATE_RSSI();
      return status; // calibrate rssi is a special case where whe should not call
      // setRfParam
      break ;
      
    case Mode::RF_RX_EXTERNAL_OOK:
      rfMode = RfMode::RX;
      buffer_RF_RX_EXTERNAL_OOK();
      Dio2Spy::start(LINE_EXTVCP_TX);
      ModeExternal::start(rfMode, board.getBaud());
      break ;
      
    case Mode::RF_TX_EXTERNAL_OOK:
      rfMode = RfMode::TX;
      buffer_RF_TX_EXTERNAL_OOK();
      Dio2Spy::start(LINE_EXTVCP_TX);
      break ;
      
     case Mode::RF_RX_EXTERNAL_FSK:
      rfMode = RfMode::RX;
      buffer_RF_RX_EXTERNAL_FSK();
      ModeFsk::start(rfMode, board.getBaud());
      break ;
      
    case Mode::RF_TX_EXTERNAL_FSK:
      rfMode = RfMode::TX;
      buffer_RF_TX_EXTERNAL_FSK();
      ModeFsk::start(rfMode, board.getBaud());
      break ;
      
    case Mode::RF_RX_INTERNAL:
      rfMode = RfMode::RX;
      buffer_RF_RX_INTERNAL();
      Dio2Spy::start(LINE_EXTVCP_TX);
      ModeTest::start(rfMode, board.getBaud());
      break ;
      
    case Mode::RF_TX_INTERNAL:
      rfMode = RfMode::TX;
      chThdSleepMilliseconds(10);
      buffer_RF_TX_INTERNAL();
      Dio2Spy::start(LINE_EXTVCP_TX);
      ModeTest::start(rfMode, board.getBaud());
      break ; 
    }
    
    if (Radio::radio->healthSurveyStart(rfMode)
	!= Rfm69Status::OK) {
      DebugTrace("Radio::radio->healthSurveyStart failed");
      status = Status::RFM69_ERROR;
      goto end;
    }
    
  end:
    return status;
  }

  const char* toAscii(Mode opMode)
  {
    static const char* ascii[] = {
    "NONE", "NORF_TX", "NORF_RX", "RF_CALIBRATE_RSSI",
    "RF_RX_EXTERNAL_OOK", "RF_TX_EXTERNAL_OOK",
    "RF_RX_EXTERNAL_FSK", "RF_TX_EXTERNAL_FSK",
    "RF_RX_INTERNAL", "RF_TX_INTERNAL"
    };
    chDbgAssert(opMode <= Ope::Mode::RF_TX_INTERNAL, "out of bound");
    return ascii[static_cast<uint8_t>(opMode)];
  }
  
   const char* toAscii(Status status)
  {
    static const char* ascii[] = {
    "OK", "RFM69_ERROR", "DATA_LINE_HOLD",
    "INTERNAL_ERROR"
    };
    chDbgAssert(status <= Ope::Status::INTERNAL_ERROR, "out of bound");
    return ascii[static_cast<uint8_t>(status)];
  }
  
}


namespace {
  void buffer_NORF_TX()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
    // embedded ftdi cable has to be customised
    // https://github.com/eswierk/ft232r_prog
    // ft232r_prog --invert_rxd
    Buffer::setMode(Buffer::Mode::INVERTED_TX); 
  }

  void buffer_NORF_RX()
  {
    palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_INPUT);
    palSetLineMode(LINE_EXTVCP_TX,
		   PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    Buffer::setMode(Buffer::Mode::RX);
  }
  
  void buffer_RF_CALIBRATE_RSSI()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
  }
  
  void buffer_RF_RX_EXTERNAL_OOK()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
    Buffer::setMode(INVERT_OOK_MODUL ? Buffer::Mode::INVERTED_RX : Buffer::Mode::RX);
  }
  
  void buffer_RF_TX_EXTERNAL_OOK()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
    Buffer::setMode(INVERT_OOK_MODUL ? Buffer::Mode::INVERTED_TX : Buffer::Mode::TX);
  }

  // in this mode, data come from antena, goes in packet fifo, is read from fifo
  // and written in the serial in regular (non swap) mode.
  // since the ftdi inverts what is sent to it, serial TX should be logic inverted
  // data is also sent on serial in inverted mode to de-invert
  void buffer_RF_RX_EXTERNAL_FSK()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
    Buffer::setMode(Buffer::Mode::INVERTED_RX);
  }
  
  // in this mode, data come from computer via ftdi or sensors via devboard and serial
  // depending on conditional compilation.
  // if serial : serial swaped, bit level non inverted, buffer is TX
  //
  // if ftdi : serial not swaped, 
  //           buffer HiZ
  void buffer_RF_TX_EXTERNAL_FSK()
  {
#if PACKET_EMISSION_READ_FROM_SERIAL
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
    Buffer::setMode(Buffer::Mode::TX);
#else
    palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_RX));
    Buffer::setMode(Buffer::Mode::HiZ);
#endif
  }
  
  void buffer_RF_RX_INTERNAL() // mode BER
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
  }
  
  void buffer_RF_TX_INTERNAL()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
  }


  void startRxTxEcho(void)
  {
    auto cb = [] (void *) {
      palWriteLine(LINE_EXTVCP_TX, palReadLine(LINE_EXTVCP_RX));
    };
    palEnableLineEvent(LINE_EXTVCP_RX, PAL_EVENT_MODE_BOTH_EDGES);
    palSetLineCallback(LINE_EXTVCP_RX, cb, NULL);
  }

  void stopRxTxEcho(void)
  {
    palDisableLineEvent(LINE_EXTVCP_RX);
  }
}
