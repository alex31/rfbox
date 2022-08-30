#include "modeTest.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"
#include "radio.hpp"
#include "etl/string.h"
#include "hardwareConf.hpp"

constexpr uint8_t preambleByte = 0xff;

namespace {
  using ErrorString = etl::string<48>;
  THD_WORKING_AREA(waAutonomousTest, 1280);
  
  static  SerialConfig meteoSerialConfig =  {
    .speed = 4800,
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN
#if DIO2_DIRECT && INVERT_UART_LEVEL && !defined(STM32F4xx_MCUCONF)
    | USART_CR2_TXINV | USART_CR2_RXINV
#endif
    ,
    .cr3 = 0
  };

  void autonomousTestWrite (void *);		
  void autonomousTestRead (void *);
  void newError(const ErrorString& es);
  
  ModeTest::Report report;
  //  virtual_timer_t vt;
  systime_t timoutTs = 0;
}



namespace ModeTest {
  float getBer(void) {
    return report.nbError * 1000.f /
      report.totalBytes;
  }

  void start(RfMode rfMode, uint32_t baud)
  {
    meteoSerialConfig.speed = baud;
    // DIO is connected on UART1_TX
#if !defined(STM32F4xx_MCUCONF)
    if (rfMode == RfMode::RX) 
      meteoSerialConfig.cr2 |= USART_CR2_SWAP;
#endif
    sdStart(&SD_METEO, &meteoSerialConfig);
    if (rfMode == RfMode::TX) {
      chThdCreateStatic(waAutonomousTest, sizeof(waAutonomousTest),
			NORMALPRIO, &autonomousTestWrite, nullptr);
    } else  if (rfMode == RfMode::RX) {
      chThdCreateStatic(waAutonomousTest, sizeof(waAutonomousTest),
			NORMALPRIO, &autonomousTestRead, nullptr);
      // chVTSetContinuous(&vt, TIME_S2I(4),
      // 	      [] (struct ch_virtual_timer *, void *) {
      // 		if (getBer() > 1000.0f) {
      // 		  NVIC_SystemReset();
      // 		}
      // 	      },
      // 	      nullptr);
    } else {
      chSysHalt("invalid rfMode");
    }
  }

  Report getReport()
  {
    report.lock();
    ModeTest::Report ret = report;
    report.unlock();
    report.reset();
    return ret;
  }
  
}



namespace {
  void newError(const ErrorString& es)
  {
#ifdef TRACE
    static systime_t lastErrorTs{0};
    static ErrorString lastError("");
    if (chTimeDiffX(lastErrorTs, chVTGetSystemTimeX()) > TIME_S2I(1)) {
      lastErrorTs = chVTGetSystemTimeX();
      chprintf(chp, "RX : %s\r\n", es.c_str());
    }
#else
      (void) es;
#endif
  }


  
  void autonomousTestWrite (void *)		
  {
    chRegSetThreadName("autonomousTestWrite");	
    // send 4 bytes preample
    static const uint8_t preamble[] = {preambleByte, preambleByte};
    uint8_t frame[32];
    uint8_t counter = 0;
    while (true) {
      for (size_t i = 0; i < sizeof(frame); i++) {
	frame[i] = counter++;
	if (counter == 160)
	  counter = 0;
      }
      sdWrite(&SD_METEO, preamble, sizeof(preamble));
      sdWrite(&SD_METEO, frame, sizeof(frame));
      chThdSleepMilliseconds(100);
    }
  }

  void autonomousTestRead (void *)		
  {
    chRegSetThreadName("autonomousTestRead");
    uint8_t expectedByte = 0;
    uint32_t zeroInRow = 0;
    char error[128];

    while (true) {
      const int c = sdGetTimeout(&SD_METEO, TIME_MS2I(200));
      if (c < 0) {
	if ((++zeroInRow) > 100) {
	  zeroInRow = 0;
	  DebugTrace("problem detected");
	  //	  RADIO::radio.calibrate();
	}

	if (chTimeDiffX(timoutTs, chVTGetSystemTimeX()) > TIME_S2I(5)) {
	  timoutTs = 0;
	} else {
	  timoutTs = chVTGetSystemTimeX();
	}
	snprintf(error, sizeof(error),
		 "rx timeout  RSSI = %.1f BER =%.1f/1000",
		 RADIO::radio.getRssi(), ModeTest::getBer());
	newError(error);
	report.timeout = true;
	report.nbError++;
	continue;
      } else if (c == 0) {
	if ((++zeroInRow) > 100) {
	  zeroInRow = 0;
	  DebugTrace("problem detected : ");
	  //	  RADIO::radio.calibrate();
	}
      } else {
	zeroInRow = 0;
	report.timeout = false;
	timoutTs = 0;
	if (c != expectedByte) {
	  if (c != preambleByte) {
	    report.nbError++;
	    // snprintf(error, sizeof(error), "expect {0x%x} got {0x%x}",
	    // 	   expectedByte, c);
	    // newError(error);
	    expectedByte = c+1;
	    if(expectedByte == 160)
	      expectedByte = 0;
	  } 
	} else {
	  if(++expectedByte == 160) {
	    report.totalBytes += expectedByte;
	    expectedByte = 0;
	    report.hasReceivedFrame = true;
	    snprintf(error, sizeof(error),
		     "frame completed  RSSI = %.1f BER =%.1f/1000",
		     RADIO::radio.getRssi(), ModeTest::getBer());
	    //	    RADIO::radio.setRestartRx(true);
	    newError(error);
	  }
	}
	
      }
      
    }
  }
}
