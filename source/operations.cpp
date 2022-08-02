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
}

namespace OPE {
  Status setMode(MODE opMode, 
		 uint32_t frequencyCarrier,
		 int8_t amplificationLevelDb) {
    Status status = Status::OK;
    OpMode rfMode = OpMode::SLEEP;
    BUFFER::setMode(BUFFER::MODE::HiZ);
    
    switch (opMode) {
    case MODE::NORF_TX:
      buffer_NORF_TX();
      break ;
      
    case MODE::NORF_RX:
      buffer_NORF_RX();
      break ;
      
    case MODE::RF_CALIBRATE_RSSI:
      rfMode = OpMode::RX;
      buffer_RF_CALIBRATE_RSSI();
      break ;
      
    case MODE::RF_RX_EXTERNAL:
      rfMode = OpMode::RX;
      buffer_RF_RX_EXTERNAL();
      break ;
      
    case MODE::RF_TX_EXTERNAL:
      rfMode = OpMode::TX;
      buffer_RF_TX_EXTERNAL();
      break ;
      
    case MODE::RF_RX_INTERNAL:
      rfMode = OpMode::RX;
      buffer_RF_RX_INTERNAL();
      break ;
      
    case MODE::RF_TX_INTERNAL:
      rfMode = OpMode::TX;
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
    BUFFER::setMode(BUFFER::MODE::TX);
#endif
  }

  void buffer_NORF_RX()
  {
    palSetLineMode(LINE_EXTVCP_RX, PAL_MODE_INPUT);
    palSetLineMode(LINE_EXTVCP_TX,
		   PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
#if DIO2_DIRECT
    BUFFER::setMode(BUFFER::MODE::RX);
#endif
  }
  
  void buffer_RF_CALIBRATE_RSSI()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
#if DIO2_DIRECT == 0
    BUFFER::setMode(BUFFER::MODE::RX);
#endif
  }
  
  void buffer_RF_RX_EXTERNAL()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
#if INVERT_UART == 0
    BUFFER::setMode(BUFFER::MODE::RX);
#else
    BUFFER::setMode(BUFFER::MODE::INVERTED_RX);
#endif
  }
  
  void buffer_RF_TX_EXTERNAL()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
#if INVERT_UART == 0
    BUFFER::setMode(BUFFER::MODE::TX);
#else
    BUFFER::setMode(BUFFER::MODE::INVERTED_TX);
#endif
  }
  
  void buffer_RF_RX_INTERNAL()
  {
    palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));
#if DIO2_DIRECT == 0
    BUFFER::setMode(INVERT_UART ? BUFFER::MODE::INVERTED_RX : BUFFER::MODE::RX);
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
    BUFFER::setMode(INVERT_UART ? BUFFER::MODE::INVERTED_TX : BUFFER::MODE::TX);
#endif    

    return ElectricalStatus::FREE;
  }
}
