#include "modeTest.hpp"
#include "ch.h"
#include "hal.h"
#include "stdutil.h"
#include "radio.hpp"
#include "etl/string.h"
#include "hardwareConf.hpp"
#include "bitIntegrator.hpp"
#include "bboard.hpp"
#include <limits>
#include <bit>

namespace {
  using ErrorString = etl::string<48>;
  constexpr uint32_t seqMax = std::numeric_limits<uint8_t>::max();
  constexpr uint32_t seqMsbMask = 1U << (std::bit_width(seqMax) - 1U);

  THD_WORKING_AREA(waAutonomousTest, 1280);
  
  SerialConfig meteoSerialConfig =  {
    .speed = baudRates[+BitRateIndex::Low],
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN |
    invertOokModulation ? (USART_CR2_TXINV | USART_CR2_RXINV) : 0,
    .cr3 = 0
  };

  bool started = false;
  void autonomousTestWrite (void *);		
  void autonomousTestRead (void *);
  constexpr uint8_t nextSeq(const uint8_t v);
  Integrator<1024> integ;
  constexpr uint8_t frameLength = 160U;

  systime_t timoutTs = 0;
}



namespace ModeTest {
  float getBer(void) {
    const float ber = integ.getAvg() * 1000.0f;
    board.setBer(ber);
    return ber;
  }

  void start(RfMode rfMode, uint32_t baud)
  {
    board.setDio2Threshold({0.48f, 0.62f});
    meteoSerialConfig.speed = baud;
    // DIO is connected on UART1_TX
    if (rfMode == RfMode::RX) 
      meteoSerialConfig.cr2 |= USART_CR2_SWAP;

    sdStart(&SD_METEO, &meteoSerialConfig);
    if (not started) {
      started = true;
      if (rfMode == RfMode::TX) {
	chThdCreateStatic(waAutonomousTest, sizeof(waAutonomousTest),
			  NORMALPRIO, &autonomousTestWrite, nullptr);
      } else  if (rfMode == RfMode::RX) {
	chThdCreateStatic(waAutonomousTest, sizeof(waAutonomousTest),
			  NORMALPRIO, &autonomousTestRead, nullptr);
      } else {
	chSysHalt("invalid rfMode");
      }
    }
  }
}



namespace {
  
  void autonomousTestWrite (void *)		
  {
    chRegSetThreadName("autonomousTestWrite");	
    uint8_t frame[32];
    uint8_t curSeq = 0;
    while (true) {
      for (size_t i = 0; i < sizeof(frame); i++) {
	frame[i] = curSeq;
	curSeq = nextSeq(curSeq);
      }
      sdWrite(&SD_METEO, frame, sizeof(frame));
      //chThdSleepMilliseconds(10);
    }
  }

  void autonomousTestRead (void *)		
  {
    chRegSetThreadName("autonomousTestRead");
    uint8_t expectedByte = 0;
    uint32_t zeroInRow = 0;
    systime_t ts = chVTGetSystemTimeX();
    bool lastByteNotExpected = false;

    while (true) {
      const int c = sdGetTimeout(&SD_METEO, TIME_MS2I(200));
      if (const systime_t now = chVTGetSystemTimeX();
	  chTimeDiffX(ts, now) > TIME_MS2I(500)) {
	ts = now;
	ModeTest::getBer();
      }
      if (c < 0) {
	if ((++zeroInRow) > 100U) {
	  zeroInRow = 0;
	  DebugTrace("problem detected : Read Timeout");
	  board.setError("Read Timeout");
	}

	if (chTimeDiffX(timoutTs, chVTGetSystemTimeX()) > TIME_S2I(5)) {
	  timoutTs = 0;
	} else {
	  timoutTs = chVTGetSystemTimeX();
	}
	board.setError("RX timeout");
	integ.push(true);
	continue;
      } else if ((c == 0) && (c != expectedByte)) {
	if ((++zeroInRow) > 10U) {
	  integ.push(true);
	  zeroInRow = 0;
	  DebugTrace("problem detected : Read only 0");
	  board.setError("Read only 0");
	  //	  Radio::radio.calibrate();
	}
      } else {
	zeroInRow = 0;
	board.clearError();
	timoutTs = 0;
	if (c != expectedByte) {
	  integ.push(true);
	  if (lastByteNotExpected) {
	    expectedByte = nextSeq(c);
	  } else {
	    expectedByte = nextSeq(expectedByte);
	  }
	  lastByteNotExpected = true;
	} else {
	  integ.push(false);
	  expectedByte = nextSeq(c);
	  lastByteNotExpected = false;
	}
      }
    }
  }

  constexpr uint8_t nextSeq(const uint8_t v)
  {
    if (v == seqMsbMask)
      return 0;
    else if (v & seqMsbMask)
      return (~v) + 1U;
    else
      return ~v;
  }
}
