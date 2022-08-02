#include "ch.h"
#include "hal.h"
#include "operations.hpp"
#include "stdutil.h"
#include "modeTest.hpp"
#include "radio.hpp"
#include "oledDisplay.hpp" 
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
      if (buffer_RF_TX_INTERNAL() != ElectricalStatus::FREE) {
	status = Status::DATA_LINE_HOLD;
	goto end;
    }
     break ; 
    }
  end:
    
    if (RADIO::radio.setRfParam(rfMode,
				frequencyCarrier,
				amplificationLevelDb)
      != Rfm69Status::OK) {
    DebugTrace("RADIO::radio.setRfParam failed");
    status = Status::RFM69_ERROR;
  }
    return status;
  }
}


namespace {
  void buffer_NORF_TX()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
#if DIO2_DIRECT
    Buffer::setMode(Buffer::Mode::TX);
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
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
#if DIO2_DIRECT == 0
    Buffer::setMode(Buffer::Mode::RX);
#endif
  }
  
  void buffer_RF_RX_EXTERNAL()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
#if INVERT_UART_LEVEL == 0
    Buffer::setMode(Buffer::Mode::RX);
#else
    Buffer::setMode(Buffer::Mode::INVERTED_RX);
#endif
  }
  
  void buffer_RF_TX_EXTERNAL()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
#if INVERT_UART_LEVEL == 0
    Buffer::setMode(Buffer::Mode::TX);
#else
    Buffer::setMode(Buffer::Mode::INVERTED_TX);
#endif
  }
  
  void buffer_RF_RX_INTERNAL()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
#if DIO2_DIRECT == 0
    Buffer::setMode(INVERT_UART_LEVEL ? Buffer::Mode::INVERTED_RX : Buffer::Mode::RX);
#endif
  }
  
  ElectricalStatus buffer_RF_TX_INTERNAL()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
#if DIO2_DIRECT == 0
    for (uint32_t i=0; i < 100; i++) {
      palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT_PULLUP);
      chThdSleepMicroseconds(1);
      if (palReadLine(LINE_EXTVCP_TX) != PAL_HIGH)
	return ElectricalStatus::HOLD;
      palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT_PULLDOWN);
      chThdSleepMicroseconds(1);
      if (palReadLine(LINE_EXTVCP_TX) != PAL_LOW)
	return ElectricalStatus::HOLD;
    }
    Buffer::setMode(INVERT_UART_LEVEL ? Buffer::Mode::INVERTED_TX : Buffer::Mode::TX);
#endif    

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
