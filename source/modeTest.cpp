#include "modeTest.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"
#include "rfm69.hpp"
#include "etl/string.h"

namespace {
  using ErrorString = etl::string<48>;
  THD_WORKING_AREA(waAutonomousTest, 512);
  
  static  SerialConfig meteoSerialConfig =  {
    .speed = 4800,
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN,
    .cr3 = 0
  };

  void autonomousTestWrite (void *);		
  void autonomousTestRead (void *);
  void newError(const ErrorString& es);
  
  OpMode opMode = OpMode::SLEEP;
}



namespace ModeTest {

  void start(OpMode _opMode, uint32_t baud)
  {
    
    opMode = _opMode;
    meteoSerialConfig.speed = baud;
    if (opMode == OpMode::TX) 
      meteoSerialConfig.cr2 |= USART_CR2_SWAP;

    sdStart(&SD1, &meteoSerialConfig);
    if (opMode == OpMode::TX) {
      chThdCreateStatic(waAutonomousTest, sizeof(waAutonomousTest),
			NORMALPRIO, &autonomousTestWrite, nullptr);
    } else  if (opMode == OpMode::RX) {
      chThdCreateStatic(waAutonomousTest, sizeof(waAutonomousTest),
			NORMALPRIO, &autonomousTestRead, nullptr);
    } else {
      chSysHalt("invalid opMode");
    }
  }
  
}



namespace {
  void newError(const ErrorString& es)
  {
    static systime_t lastErrorTs{0};
    static ErrorString lastError("");
    if (chTimeDiffX(lastErrorTs, chVTGetSystemTimeX()) > TIME_S2I(1)) {
      lastErrorTs = chVTGetSystemTimeX();
      chprintf(chp, "RX : %s\r\n", es.c_str());
    }
  }


  
  void autonomousTestWrite (void *)		
  {
    chRegSetThreadName("autonomousTestWrite");	
    // send 4 bytes preample
    static const uint8_t preamble[] = {0xaa, 0xaa};
    uint8_t frame[32];
    uint8_t counter = 0;
    while (true) {
      for (size_t i = 0; i < sizeof(frame); i++) {
	frame[i] = counter++;
	if (counter == 160)
	  counter = 0;
      }
      sdWrite(&SD1, preamble, sizeof(preamble));
      sdWrite(&SD1, frame, sizeof(frame));
      chThdSleepMilliseconds(100);
    }
  }

  void autonomousTestRead (void *)		
  {
    chRegSetThreadName("autonomousTestRead");	
    uint32_t nbConsecutiveError = 0;
    uint8_t expectedByte = 0;
    
    while (true) {
      const int c = sdGetTimeout(&SD1, TIME_MS2I(200));
      if (c < 0) {
	newError("timeout");
	continue;
      }

      if (c != expectedByte) {
	nbConsecutiveError++;
	char error[48];
	snprintf(error, sizeof(error), "expect {%c} got {%c} [%lu errors]",
		 expectedByte, c,  nbConsecutiveError);
	newError(error);
	if(++expectedByte == 160)
	  expectedByte = 0;
      } else {
	if(++expectedByte == 160) {
	  expectedByte = 0;
	  newError("frame completed");
	}
 	nbConsecutiveError = 0;
      }
      
    }
    
  }
}
