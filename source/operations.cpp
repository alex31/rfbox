#include "ch.h"
#include "hal.h"
#include "operations.hpp"
#include "stdutil.h"
#include "modeTest.hpp"
#include "radio.hpp"
#include "buffer.hpp"
#include "hardwareConf.hpp"

namespace {
  enum class ElectricalStatus {FREE, HOLD};

  void buffer_NORF_TX();
  void buffer_NORF_RX();
  void buffer_RF_CALIBRATE_RSSI();
  void buffer_RF_RX_EXTERNAL();
  void buffer_RF_TX_EXTERNAL();
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
      
    case Mode::RF_RX_EXTERNAL:
      rfMode = RfMode::RX;
      buffer_RF_RX_EXTERNAL();
      break ;
      
    case Mode::RF_TX_EXTERNAL:
      rfMode = RfMode::TX;
      buffer_RF_TX_EXTERNAL();
      break ;
      
    case Mode::RF_RX_INTERNAL:
      rfMode = RfMode::RX;
      buffer_RF_RX_INTERNAL();
      break ;
      
    case Mode::RF_TX_INTERNAL:
      rfMode = RfMode::TX;
      Radio::radio.setRfParam(rfMode,
			      frequencyCarrier,
			      amplificationLevelDb);
      chThdSleepMilliseconds(10);
      if (buffer_RF_TX_INTERNAL() != ElectricalStatus::FREE) {
	DebugTrace("meteo uart data line not free");
	status = Status::DATA_LINE_HOLD;
	Radio::radio.setRfParam(RfMode::SLEEP,
				frequencyCarrier,
				amplificationLevelDb);

	goto end;
      }
      break ; 
    }
    
    if (Radio::radio.setRfParam(rfMode,
				frequencyCarrier,
				amplificationLevelDb)
	!= Rfm69Status::OK) {
      DebugTrace("Radio::radio.setRfParam failed");
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
    "RF_RX_EXTERNAL", "RF_TX_EXTERNAL", "RF_RX_INTERNAL",
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
#if DIO2_DIRECT
    // embedded ftdi cable has to be customised
    // https://github.com/eswierk/ft232r_prog
    // ft232r_prog --invert_rxd
    Buffer::setMode(Buffer::Mode::INVERTED_TX); 
#endif
  }

  void buffer_NORF_RX()
  {
    palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_INPUT);
    palSetLineMode(LINE_EXTVCP_TX,
		   PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
#if DIO2_DIRECT
    Buffer::setMode(Buffer::Mode::RX);
#endif
  }
  
  void buffer_RF_CALIBRATE_RSSI()
  {
    palSetLineMode(LINE_MCU_RX, PAL_MODE_INPUT);
#if DIO2_DIRECT == false
    Buffer::setMode(Buffer::Mode::RX);
#endif
  }
  
  void buffer_RF_RX_EXTERNAL()
  {
    palSetLineMode(LINE_MCU_RX, PAL_MODE_ALTERNATE(AF_LINE_MCU_RX));
#if INVERT_UART_LEVEL == false
    Buffer::setMode(Buffer::Mode::RX);
#else
    Buffer::setMode(Buffer::Mode::INVERTED_RX);
#endif
  }
  
  void buffer_RF_TX_EXTERNAL()
  {
    palSetLineMode(LINE_MCU_RX, PAL_MODE_INPUT);
#if INVERT_UART_LEVEL == false
    Buffer::setMode(Buffer::Mode::TX);
#else
    Buffer::setMode(Buffer::Mode::INVERTED_TX);
#endif
  }
  
  void buffer_RF_RX_INTERNAL() // mode BER
  {
    palSetLineMode(LINE_MCU_RX, PAL_MODE_ALTERNATE(AF_LINE_MCU_RX));
#if DIO2_DIRECT == false
    Buffer::setMode(INVERT_UART_LEVEL ? Buffer::Mode::INVERTED_RX : Buffer::Mode::RX);
#endif
  }
  
  ElectricalStatus buffer_RF_TX_INTERNAL()
  {
#if DIO2_DIRECT == false
    for (uint32_t i=0; i < 100; i++) {
      palSetLineMode(LINE_MCU_RX, PAL_MODE_INPUT_PULLUP);
      chThdSleepMicroseconds(1);
      if (palReadLine(LINE_MCU_RX) != PAL_HIGH) {
	DebugTrace("iteration %lu read low instead high", i);
	return ElectricalStatus::HOLD;
      }
      palSetLineMode(LINE_MCU_RX, PAL_MODE_INPUT_PULLDOWN);
      chThdSleepMicroseconds(1);
      if (palReadLine(LINE_MCU_RX) != PAL_LOW) {
	DebugTrace("iteration %lu read high instead low", i);
	return ElectricalStatus::HOLD;
      }
    }
    Buffer::setMode(INVERT_UART_LEVEL ? Buffer::Mode::INVERTED_TX :
		    Buffer::Mode::TX);
#endif    
    palSetLineMode(LINE_MCU_RX, PAL_MODE_ALTERNATE(AF_LINE_MCU_RX));
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
