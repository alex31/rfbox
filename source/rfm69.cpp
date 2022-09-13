#include "rfm69.hpp"
#include "stdutil.h"
#include "operations.hpp"
#include "hardwareConf.hpp"
#include "dio2Spy.hpp"
#include "etl/string.h"
#include "bboard.hpp"

#define SWAP_ENDIAN24(x) __builtin_bswap32(static_cast<uint32_t>(x) << 8)

namespace {
  constexpr uint8_t readMask =  0x0;
  constexpr uint8_t writeMask = 0x80;
  MUTEX_DECL(calMtx);
  Rfm69Rmap regCheck;
}

void Rfm69Spi::reset(void)
{
  spiAcquireBus(&spid);
  if (spid.state >= SPI_READY) {
    const SPIConfig* cfg = spid.config;
    spiStop(&spid);
    spiStart(&spid, cfg);
  }
  
  palSetLine(lineReset);
  palSetLineMode(lineReset, PAL_MODE_OUTPUT_PUSHPULL);
  chThdSleepMicroseconds(100);
  palSetLineMode(lineReset, PAL_MODE_INPUT_ANALOG);
  chThdSleepMilliseconds(10);
  spiReleaseBus(&spid);
}

Rfm69Status Rfm69Spi::init(const SPIConfig& spiCfg)
{
  reset();
  Rfm69Status status = Rfm69Status::OK;
  spiStart(&spid, &spiCfg);
  cacheRead(Rfm69RegIndex::First, sizeof(reg));
  const auto bitRate = SWAP_ENDIAN16(reg.bitrate);
  if (bitRate != 0x1a0b) {
    DebugTrace("initial reg.bitRate value 0x%x"
	       "!= documented init value 0x1a0b", bitRate);
    status = Rfm69Status::INIT_ERROR;
  }

  return status;
}

void Rfm69Spi::cacheRead(Rfm69RegIndex idx, size_t len)
{
  spiAcquireBus(&spid);
  do {
    spiSelect(&spid);
    const uint8_t slaveAddr = static_cast<uint8_t>(idx) | readMask;
    spiSend(&spid, 1, &slaveAddr);
    spiReceive(&spid, len, const_cast<uint8_t *>(reg.raw) +
	       static_cast<uint32_t>(idx));
    spiUnselect(&spid);
    memcpy(&regCheck, const_cast<Rfm69Rmap *>(&reg), sizeof(reg));
    spiSelect(&spid);
    spiSend(&spid, 1, &slaveAddr);
    spiReceive(&spid, len, const_cast<uint8_t *>(reg.raw) +
	       static_cast<uint32_t>(idx));
    spiUnselect(&spid);
    chThdSleepMicroseconds(1);
  }
  while (memcmp(&regCheck, const_cast<Rfm69Rmap *>(&reg),
		sizeof(reg)) != 0);
   // chThdSleepMicroseconds(10);
  spiReleaseBus(&spid);
  // chThdSleepMicroseconds(10);
}

void Rfm69Spi::cacheWrite(Rfm69RegIndex idx, size_t len)
{
  spiAcquireBus(&spid);
  spiSelect(&spid);
  // chThdSleepMicroseconds(10);
  const uint8_t slaveAddr = static_cast<uint8_t>(idx) | writeMask;
  spiSend(&spid, 1, &slaveAddr);
  spiSend(&spid, len, const_cast<uint8_t *>(reg.raw) + static_cast<uint32_t>(idx));
  // chThdSleepMicroseconds(10);
  spiUnselect(&spid);
  spiReleaseBus(&spid);
  // chThdSleepMicroseconds(10);
}

void Rfm69Spi::saveReg(void)
{
  memcpy(&regSave, const_cast<const Rfm69Rmap *>(&reg), sizeof(reg));
}

void Rfm69Spi::restoreReg(void)
{
  memcpy(const_cast<Rfm69Rmap *>(&reg), const_cast<const Rfm69Rmap *>(&regSave),
	 sizeof(reg));
}

// if one want to try CONTINUOUS_SYNC here : frame preamble
// 16 bits patern has to be 0xAAAA instead of 0xFFFF
Rfm69Status Rfm69OokRadio::init(const SPIConfig& spiCfg)
{
  Rfm69Status status = rfm69.init(spiCfg);
  if (status != Rfm69Status::OK)
    goto end;

  if ((status = calibrate()) != Rfm69Status::OK)
    goto end;
  
  rfm69.reg.opMode_mode = mode; // mode or RfMode::FS
  rfm69.reg.opMode_sequencerOff = 0; // sequencer is activated
  rfm69.reg.opMode_listenOn = 0; 
  rfm69.reg.datamodul_dataMode = DataMode::CONTINUOUS_NOSYNC;
  rfm69.reg.bitrate = SWAP_ENDIAN16(xtalHz / board.getBaud());
  rfm69.reg.datamodul_shaping = DataModul::OOK_NOSHAPING;
  rfm69.cacheWrite(Rfm69RegIndex::DataModul,
		   Rfm69RegIndex::AesKey - Rfm69RegIndex::DataModul);
  rfm69.cacheWrite(Rfm69RegIndex::RfMode);
  waitReady();
 end:
  return status;
}

float Rfm69OokRadio::getRssi()
{
  rfm69.cacheRead(Rfm69RegIndex::RssiConfig, 2);
  const float rssi = -rfm69.reg.rssi / 2.0f;
  board.setRssi(rssi);
  return rssi;
}

void	 Rfm69OokRadio::setBaudRate(uint32_t br)
{
  rfm69.reg.bitrate = SWAP_ENDIAN16(xtalHz / br);
  rfm69.cacheWrite(Rfm69RegIndex::Bitrate);
}

void Rfm69OokRadio::coldReset()
{
  do {
    rfm69.reset();
    rfm69.cacheWrite(Rfm69RegIndex::DataModul, Rfm69RegIndex::Last - Rfm69RegIndex::DataModul);
    waitReady();
    setModeAndWait(mode);
    calibrate();
    rfm69.cacheRead(Rfm69RegIndex::RfMode);
    rfm69.cacheRead(Rfm69RegIndex::IrqFlags1);
  } while  ((mode == RfMode::RX) and (rfm69.reg.irqFlags1 != 0xD8));
}


void Rfm69OokRadio::checkModeMismatch()
{
  rfm69.cacheRead(Rfm69RegIndex::RfMode);
  if ((mode != RfMode::SLEEP) and (rfm69.reg.opMode_mode != mode)) {
    DebugTrace("mismatch found mode %u instead %u, reset...",
	       static_cast<uint16_t>(rfm69.reg.opMode_mode),
	       static_cast<uint16_t>(mode));
    board.setError("Radio lockout");
    coldReset();
  } else if (const float av = Dio2Spy::getAverageLevel();
	     (mode == RfMode::RX) and ((av < 0.30f) or (av > 0.70f))) {
    DebugTrace("dio2 average not valid = %.2f", av);
    board.setError("DIO2 Avg Err");
    coldReset();
  } else {
    board.clearError();
  }
}

void Rfm69OokRadio::checkRestartRxNeeded()
{
  const float rssi = getRssi();
  const float lnaGain = getLnaGain();
  if ( ((rssi < -60.0f ) and (lnaGain < -24.0f))
      or
       ((rssi > -60.0f) and (lnaGain > -24.0f)) ) {
    DebugTrace("RSSI change : setRestartRx");
    //    coldReset();
    setRestartRx(true);
  }
}

Rfm69Status Rfm69OokRadio::calibrate()
{
  chMtxLock(&calMtx);
  const systime_t ts = chVTGetSystemTimeX();

 
  rfm69.reg.osc1_calibStart = true;
  rfm69.cacheWrite(Rfm69RegIndex::Osc1);
  do 
    rfm69.cacheRead(Rfm69RegIndex::Osc1);
  while ((not rfm69.reg.osc1_calibDone) and
	 (chTimeDiffX(ts, chVTGetSystemTimeX()) < TIME_S2I(1)));
  if (not rfm69.reg.osc1_calibDone) {
    DebugTrace("ERR: osc1 calibration timeout");
  }

  rfm69.cacheRead(Rfm69RegIndex::RfMode);
  rfm69.cacheRead(Rfm69RegIndex::IrqFlags1);
  if (mode == RfMode::RX) {
    DebugTrace("calibration ends mode = 0x%x; dio2 avg = %.2f",
	       static_cast<uint16_t>(rfm69.reg.opMode_mode),
	       Dio2Spy::getAverageLevel());
  } else {
    DebugTrace("calibration ends mode = 0x%x",
	       static_cast<uint16_t>(rfm69.reg.opMode_mode));
  }
  humanDisplayFlags();
  chMtxUnlock(&calMtx);

  return rfm69.reg.osc1_calibDone ? Rfm69Status::OK : Rfm69Status::TIMOUT;
}

/*
  continous mode without gaussian filter : asynchronous mode without clock

  In OOK, automatic gain control will work if frame begin several all 1
  words : that mean that frame must begin with 4 0xFF before sync beacon

 */
Rfm69Status Rfm69OokRadio::setRfParam(RfMode _mode,
				      uint32_t frequencyCarrier,
				      int8_t amplificationLevelDb)
{
  mode = _mode;
  if (rfHealthSurveyThd != nullptr) {
    chThdTerminate(rfHealthSurveyThd);
    while (not chThdTerminatedX(rfHealthSurveyThd)) {
      chThdSleepMilliseconds(10);
    }
    chThdRelease(rfHealthSurveyThd);
    rfHealthSurveyThd = nullptr;
  }

  if (mode == RfMode::TX) {
    setFrequencyCarrier(frequencyCarrier);
    setPowerAmp(0b001, RampTime::US_20, amplificationLevelDb);
    setOcp_on(false);
    
  } else  if (mode == RfMode::RX) {
    setFrequencyCarrier(frequencyCarrier);
    setReceptionTuning();
  }  
  setLna(LnaGain::AGC, LnaInputImpedance::OHMS_50);
  auto status = setModeAndWait(mode);
  DebugTrace("wait status for mode[%lx] = %lx",
	     static_cast<uint32_t>(mode),
	     static_cast<uint32_t>(status));
  if (status != Rfm69Status::OK)
    goto exit;
  // optional : to be tested, optimisation of floor threshold
  // works only in the absence of module emitting !!
  if (mode == RfMode::RX) {
    // desactivate AGC, use minimal gain (to be tested)
    //    calibrateRssiThresh();
    DebugTrace("current lna gain = %d ... RSSI = %.1f",
	       getLnaGain(), getRssi());
  }

  //  rfm69.saveReg();

  if ((mode == RfMode::RX) or (mode == RfMode::TX)) 
    rfHealthSurveyThd = chThdCreateStatic(waSurvey, sizeof(waSurvey),
					 NORMALPRIO - 1, &rfHealthSurvey, this);
  
 exit:
  return status;
}

Rfm69Status Rfm69OokRadio::waitReady(void)
{
  systime_t start = chVTGetSystemTimeX();

  const RfMode rfmode = getRfMode();
  
  while (chTimeDiffX(start, chVTGetSystemTimeX()) < TIME_MS2I(1000)) {
    rfm69.cacheRead(Rfm69RegIndex::IrqFlags1);
    if (rfmode == RfMode::RX) {
      if (rfm69.reg.irqFlags1 == 0xD8)
	return Rfm69Status::OK;
    } else  if (rfmode == RfMode::TX) {
      if (rfm69.reg.irqFlags_txReady)
	return Rfm69Status::OK;
    } else if (rfmode == RfMode::FS) {
      if (rfm69.reg.irqFlags_modeReady and rfm69.reg.irqFlags_pllLock)
	return Rfm69Status::OK;
    } else {
      if (rfm69.reg.irqFlags_modeReady)
	return Rfm69Status::OK;
    }
    chThdSleepMilliseconds(1);
  }
  DebugTrace("waitReady timout for mode %x",
	     static_cast<uint16_t>(rfmode));
  return Rfm69Status::TIMOUT;
}



void Rfm69OokRadio::setPowerAmp(uint8_t pmask, RampTime rt, int8_t gain)
{
  chDbgAssert((gain >= -13) and (gain <= 18),
	      "out of bound amplifier level");
  
  rfm69.reg.paLevel_pa0On = (pmask & 0b001) != 0;
  rfm69.reg.paLevel_pa1On = (pmask & 0b010) != 0;
  rfm69.reg.paLevel_pa2On = (pmask & 0b100) != 0;
  // power outpul level is from -13db to 18 db
  rfm69.reg.paLevel_outputPower = gain + 13; 
  
  rfm69.reg.paRamp = rt;
  rfm69.cacheWrite(Rfm69RegIndex::PaLevel, 2);
}

void Rfm69OokRadio::setOokPeak(ThresholdType t, ThresholdDec d,
			       ThresholdStep s)
{
  rfm69.reg.ookPeak_type = t;
  rfm69.reg.ookPeak_threshDec = d;
  rfm69.reg.ookPeak_threshStep = s;
  rfm69.cacheWrite(Rfm69RegIndex::OokPeak);
}

void Rfm69OokRadio::setRxBw(BandwithMantissa bm, uint8_t exp,
			    uint8_t dccFreq)
{
  rfm69.reg.rxBw_mant =  bm;
  rfm69.reg.rxBw_exp = exp;
  rfm69.reg.rxBw_dccFreq = dccFreq; // default 4 % of rxbw
  rfm69.cacheWrite(Rfm69RegIndex::RxBw);
}

void Rfm69OokRadio::setFrequencyCarrier(uint32_t frequencyCarrier)
{
  chDbgAssert((frequencyCarrier > 100'000'000) and (frequencyCarrier < 1'000'000'000),
	      "out of bound frequency carrier");
  rfm69.reg.frf = SWAP_ENDIAN24(frequencyCarrier / synthStepHz);
  rfm69.cacheWrite(Rfm69RegIndex::Frf, 3);

}

void Rfm69OokRadio::setReceptionTuning(void)
{
  // should try ThresholdType::FIXED;
  // setOokPeak(ThresholdType::AVERAGE, ThresholdDec::SIXTEEN_TIMES,
  // 	     ThresholdStep::DB_2);
  setOokPeak(ThresholdType::PEAK, ThresholdDec::ONE,
	     ThresholdStep::DB_0P5);
  setOokFix_threshold(0);
  setLowBetaOn(false);
  setDagc(FadingMargin::IMPROVE_LOW_BETA_OFF);
  setAutoRxRestart(false);
  
  // settings for RxBw = 20Khz in OOK
  if (board.getBaud() == baudLow)
    setRxBw(BandwithMantissa::MANT_24, 4, 2); /* 10 Khz bandwith, dccfreq default 4 % of rxbx */
  else
    setRxBw(BandwithMantissa::MANT_24, 2, 2); /* 42Khz bandwith, dccfreq default 4 % of rxbx */
  
  // in case of false '1', raise the value to 0xFF cf datasheet p61
  setRssi_threshold(0xE4);
  DebugTrace("rfm69.reg.rssiThresh = %d", rfm69.reg.rssiThresh);

  // automatic frequency correction activated
  setAfc_autoOn(true);
}


// floor threshold optimisation, see fig 12 of RFM69W datasheet
// 27 step to be tested in maximum one second : 35 ms for each step
void Rfm69OokRadio::calibrateRssiThresh(void)
{
  auto isDioStableLow = [] {
    const bool low = palReadLine(LINE_MCU_RX) == PAL_LOW; 
    DebugTrace("level = %s", low ? "LOW" : "HIGH");
    if (low) {
      const bool stable = palWaitLineTimeout(LINE_MCU_RX, TIME_MS2I(35)) == MSG_TIMEOUT;
      DebugTrace("stable = %s", stable ? "YES" : "NO");
      return stable;
    } else {
      return false;
    }
  };

  Ope::setMode(Ope::Mode::RF_CALIBRATE_RSSI, 0, 0);
  
  palEnableLineEvent(LINE_MCU_RX, PAL_EVENT_MODE_BOTH_EDGES);
  for (uint16_t t = 0xB0; t <= 0xFF; t++) {
    rfm69.reg.rssiThresh = t;
    DebugTrace("rfm69.reg.rssiThresh = %d", rfm69.reg.rssiThresh);
    rfm69.cacheWrite(Rfm69RegIndex::RssiThresh);
    if (isDioStableLow())
      break;
  }
  palDisableLineEvent(LINE_MCU_RX);

  // rfm69.reg.ookPeak_threshDec = ThresholdDec::EIGHT_TIMES;
  // rfm69.reg.ookPeak_threshStep = ThresholdStep::DB_0P5;
  // rfm69.cacheWrite(Rfm69RegIndex::OokPeak);
}

void Rfm69OokRadio::setLna(LnaGain gain, LnaInputImpedance imp)
{
  rfm69.reg.lna_gain = gain;
  rfm69.reg.lna_zIn  = imp;
  rfm69.cacheWrite(Rfm69RegIndex::Lna);
}

int8_t Rfm69OokRadio::getLnaGain(void)
{
  rfm69.cacheRead(Rfm69RegIndex::Lna);
  int8_t lna_g = 127;
  switch(rfm69.reg.lna_currentGain) {
  case LnaGain::HIGHEST : lna_g = 0; break;
  case LnaGain::MINUS_6 : lna_g = -6; break;
  case LnaGain::MINUS_12 : lna_g = -12; break;
  case LnaGain::MINUS_24 : lna_g = -24; break;
  case LnaGain::MINUS_36 : lna_g = -36; break;
  case LnaGain::MINUS_48 : lna_g = -48; break;
  default:		   lna_g = 127;
  }
  board.setLnaGain(lna_g);
  return lna_g;
}

// void Rfm69OokRadio::rfHealthSurvey(void *arg)
// {
//   Rfm69OokRadio *radio = static_cast<Rfm69OokRadio *>(arg);
//   uint32_t successiveRxNotReady = 0;
  
//   chRegSetThreadName("RX Ready survey");
//   while (not chThdShouldTerminateX()) {
//     const bool rxReady = radio->getRxReady();
//     if (not rxReady)
//       successiveRxNotReady++;
//     else
//       successiveRxNotReady = 0;
    
//     if (successiveRxNotReady > 5) {
//       DebugTrace("RxReady false for 0.5 second: force calibration");
//       radio->calibrate();
//       chThdSleepMilliseconds(200);
//     }
//     chThdSleepMilliseconds(50);
//   }
//   chThdExit(MSG_OK);
// }

void Rfm69OokRadio::humanDisplayFlags(void)
{
  rfm69.cacheRead(Rfm69RegIndex::IrqFlags1);
  etl::string<80> flags = "FLAGS= ";
  flags += rfm69.reg.irqFlags_autoMode ? "AUTO_MODE" : "auto_mode";
  flags += ", ";
  flags += rfm69.reg.irqFlags_timeOut ? "TIME_OUT" : "time_out";
  flags += ", ";
  flags += rfm69.reg.irqFlags_rssi ? "RSSI" : "rssi";
  flags += ", ";
  flags += rfm69.reg.irqFlags_pllLock ? "PLL_LOCK" : "pll_lock";
  flags += ", ";
  flags += rfm69.reg.irqFlags_txReady ? "TX_READY" : "tx_ready";
  flags += ", ";
  flags += rfm69.reg.irqFlags_rxReady ? "RX_READY" : "rx_ready";
  flags += ", ";
  flags += rfm69.reg.irqFlags_modeReady ? "MODE_READY" : "mode_ready";
  DebugTrace("%s", flags.c_str());
}


Rfm69Status Rfm69OokRadio::setModeAndWait(RfMode nmode)
{
  setRfMode(nmode);
  return waitReady();
}


void Rfm69OokRadio::rfHealthSurvey(void *arg)
{
  Rfm69OokRadio *radio = static_cast<Rfm69OokRadio *>(arg);
  chRegSetThreadName("RF Health survey");
  uint32_t calCount = 0, cmmCount = 0;
  
  while (not chThdShouldTerminateX()) {
    chThdSleepMilliseconds(100);
    if (++cmmCount > 10) {
      radio->checkModeMismatch();
      radio->checkRestartRxNeeded();
      cmmCount = 0;
    }
    if (++calCount > 600) {
      radio->calibrate();
      calCount = 0;
    }
  }
  chThdExit(MSG_OK);
}

THD_WORKING_AREA(Rfm69OokRadio::waSurvey, 512);
