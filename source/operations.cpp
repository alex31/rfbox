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
  ElectricalStatus buffer_RF_TX_INTERNAL();

  void startRxTxEcho(void);
  void stopRxTxEcho(void);
}

namespace Ope {
  Status setMode(Mode opMode, 
		 uint32_t frequencyCarrier,
		 int8_t amplificationLevelDb) {
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
      Radio::radio->setRfParam(rfMode,
			      frequencyCarrier,
			      amplificationLevelDb);
      chThdSleepMilliseconds(10);
      if (buffer_RF_TX_INTERNAL() != ElectricalStatus::FREE) {
	DebugTrace("meteo uart data line not free");
	status = Status::DATA_LINE_HOLD;
	Radio::radio->setRfParam(RfMode::SLEEP,
				frequencyCarrier,
				amplificationLevelDb);

	goto end;
      }
      Dio2Spy::start(LINE_EXTVCP_TX);
      ModeTest::start(rfMode, board.getBaud());
      break ; 
    }
    
    if (Radio::radio->setRfParam(rfMode,
				frequencyCarrier,
				amplificationLevelDb)
	!= Rfm69Status::OK) {
      DebugTrace("Radio::radio->setRfParam failed");
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
    "RF_RX_EXTERNAL_OOK", "RF_TX_EXTERNAL_OOK", "RF_RX_INTERNAL",
    "RF_TX_INTERNAL"
    };
    return ascii[static_cast<uint8_t>(opMode)];
  }
  
   const char* toAscii(Status status)
  {
    static const char* ascii[] = {
    "OK", "RFM69_ERROR", "DATA_LINE_HOLD",
    "INTERNAL_ERROR"
    };
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
#if INVERT_UART_LEVEL == false
    Buffer::setMode(Buffer::Mode::RX);
#else
    Buffer::setMode(Buffer::Mode::INVERTED_RX);
#endif
  }
  
  void buffer_RF_TX_EXTERNAL_OOK()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
#if INVERT_UART_LEVEL == false
    Buffer::setMode(Buffer::Mode::TX);
#else
    Buffer::setMode(Buffer::Mode::INVERTED_TX);
#endif
  }
  
  void buffer_RF_RX_EXTERNAL_FSK()
  {
    palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_RX));
    Buffer::setMode(Buffer::Mode::HiZ);
  }
  
  void buffer_RF_TX_EXTERNAL_FSK()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
    Buffer::setMode(Buffer::Mode::RX);
  }
  
  void buffer_RF_RX_INTERNAL() // mode BER
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
  }
  
  ElectricalStatus buffer_RF_TX_INTERNAL()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
    return ElectricalStatus::FREE;
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
