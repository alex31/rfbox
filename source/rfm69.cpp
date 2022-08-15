#include "rfm69.hpp"
#include "stdutil.h"
#include "operations.hpp"
#include "hardwareConf.hpp"

#define SWAP_ENDIAN24(x) __builtin_bswap32(static_cast<uint32_t>(x) << 8)

namespace {
  static constexpr uint8_t readMask = 0;
  static constexpr uint8_t writeMask = 0x80;
}

void Rfm69Spi::reset(void)
{
  palSetLineMode(lineReset, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLine(lineReset);
  chThdSleepMicroseconds(100);
  palClearLine(lineReset);
  palSetLineMode(lineReset, PAL_MODE_INPUT_ANALOG);
  chThdSleepMilliseconds(10);
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
  spiSelect(&spid);
  uint8_t slaveAddr = static_cast<uint8_t>(idx) | readMask;
  spiSend(&spid, 1, &slaveAddr);
  spiReceive(&spid, len, reg.raw + static_cast<uint8_t>(idx));
  spiUnselect(&spid);
  spiReleaseBus(&spid);
}

void Rfm69Spi::cacheWrite(Rfm69RegIndex idx, size_t len)
{
  spiAcquireBus(&spid);
  spiSelect(&spid);
  uint8_t slaveAddr = static_cast<uint8_t>(idx) | writeMask;
  spiSend(&spid, 1, &slaveAddr);
  spiSend(&spid, len, reg.raw + static_cast<uint8_t>(idx));
  spiUnselect(&spid);
  spiReleaseBus(&spid);
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
  rfm69.reg.datamodul_shaping = DataModul::OOK_NOSHAPING;
  rfm69.cacheWrite(Rfm69RegIndex::RfMode,
		   Rfm69RegIndex::AesKey - Rfm69RegIndex::First);
  waitReady();
 end:
  return status;
}

float Rfm69OokRadio::getRssi()
{
  rfm69.cacheRead(Rfm69RegIndex::RssiConfig, 2);
  return (-rfm69.reg.rssi / 2.0f);
 }

Rfm69Status Rfm69OokRadio::calibrate()
{
  systime_t ts = chVTGetSystemTimeX();
  
  const auto saveMode = rfm69.reg.opMode_mode;
  const bool hasToSaveRestore = saveMode != RfMode::STDBY;
  if (hasToSaveRestore) {
    rfm69.reg.opMode_mode = RfMode::STDBY;
    rfm69.cacheWrite(Rfm69RegIndex::RfMode);
    chThdSleepMilliseconds(50);
  }
  
  rfm69.reg.osc1_calibStart = true;
  rfm69.cacheWrite(Rfm69RegIndex::Osc1);
  do {
    rfm69.cacheRead(Rfm69RegIndex::Osc1);
  } while ((not rfm69.reg.osc1_calibDone) or
	   (chTimeDiffX(chVTGetSystemTimeX(), ts) < TIME_S2I(1)));

  if (hasToSaveRestore) {
    rfm69.reg.opMode_mode = saveMode;
    rfm69.cacheWrite(Rfm69RegIndex::Osc1);
  }

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
  if (mode == RfMode::TX) {
    setFrequencyCarrier(frequencyCarrier);
    setPowerAmp(0b001, RampTime::US_20, amplificationLevelDb);
    setOcp_on(false);
    
  } else  if (mode == RfMode::RX) {
    setFrequencyCarrier(frequencyCarrier);
    setReceptionTuning();
  }  
  rfm69.reg.opMode_mode = mode;
  rfm69.cacheWrite(Rfm69RegIndex::RfMode);

  auto status = waitReady();
  DebugTrace("wait status for mode[%lx] = %lx",
	     static_cast<uint32_t>(mode),
	     static_cast<uint32_t>(status));
  
  // optional : to be tested, optimisation of floor threshold
  // works only in the absence of module emitting !!
  if (mode == RfMode::RX) {
    // desactivate AGC, use minimal gain (to be tested)
    setLna(LnaGain::AGC, LnaInputImpedance::OHMS_50);
    //    calibrateRssiThresh();
    DebugTrace("current lna gain = %d ... RSSI = %.1f",
	       getLnaGain(), getRssi());
    chThdSleepSeconds(2);
  }
  
  
  return status;
}

Rfm69Status Rfm69OokRadio::waitReady(void)
{
  systime_t start = chVTGetSystemTimeX();
  
  while (chTimeDiffX(start, chVTGetSystemTimeX()) < TIME_MS2I(1000)) {
    rfm69.cacheRead(Rfm69RegIndex::IrqFlags1);
    if (mode == RfMode::RX) {
      if (rfm69.reg.irqFlags_rxReady)
	return Rfm69Status::OK;
    } else  if (mode == RfMode::TX) {
      if (rfm69.reg.irqFlags_txReady)
	return Rfm69Status::OK;
    } else if (mode == RfMode::FS) {
      if (rfm69.reg.irqFlags_modeReady and rfm69.reg.irqFlags_pllLock)
	return Rfm69Status::OK;
    } else {
      if (rfm69.reg.irqFlags_modeReady)
	return Rfm69Status::OK;
    }
    chThdSleepMilliseconds(1);
  }
  DebugTrace("waitReady timout for mode %x",
	     static_cast<uint16_t>(mode));
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
  rfm69.reg.rxBw_dccFreq = dccFreq; // default 4 % of rxbx -> 800hz
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
  setOokPeak(ThresholdType::AVERAGE, ThresholdDec::SIXTEEN_TIMES,
	     ThresholdStep::DB_2);
  setOokFix_threshold(0);
  setLowBetaOn(true);
  setDagc(FadingMargin::IMPROVE_LOW_BETA_ON);
  
  // settings for RxBw = 20Khz in OOK
  setRxBw(BandwithMantissa::MANT_24, 4, 2/* default 4 % of rxbx -> 800hz*/);
  
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
  switch(rfm69.reg.lna_currentGain) {
  case LnaGain::HIGHEST : return 0;
  case LnaGain::MINUS_6 : return -6;
  case LnaGain::MINUS_12 : return -12;
  case LnaGain::MINUS_24 : return -24;
  case LnaGain::MINUS_36 : return -36;
  case LnaGain::MINUS_48 : return -48;
  default:		   return 127;
  }
}
