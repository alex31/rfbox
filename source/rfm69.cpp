#include "rfm69.hpp"
#include "stdutil.h"
#include "notGate.hpp"

#define SWAP_ENDIAN24(x) __builtin_bswap32(static_cast<uint32_t>(x) << 8)

namespace {
  static constexpr uint8_t readMask = 0;
  static constexpr uint8_t writeMask = 0x80;
}

void Rfm69Spi::reset(void)
{
  palSetLine(lineReset);
  palSetLineMode(lineReset, PAL_MODE_OUTPUT_PUSHPULL);
  chThdSleepMilliseconds(1);
  palSetLineMode(lineReset, PAL_MODE_INPUT);
  chThdSleepMilliseconds(20);
}

Rfm69Status Rfm69Spi::init(const SPIConfig& spiCfg)
{
  reset();
  Rfm69Status status = Rfm69Status::OK;
  spiStart(&spid, &spiCfg);
  cacheRead(Rfm69RegIndex::First, sizeof(reg.raw));
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
  spiSelect(&spid);
  uint8_t slaveAddr = static_cast<uint8_t>(idx) | readMask;
  spiSend(&spid, 1, &slaveAddr);
  spiReceive(&spid, len, reg.raw + static_cast<uint8_t>(idx));
  spiUnselect(&spid);
}

void Rfm69Spi::cacheWrite(Rfm69RegIndex idx, size_t len)
{
  spiSelect(&spid);
  uint8_t slaveAddr = static_cast<uint8_t>(idx) | writeMask;
  spiSend(&spid, 1, &slaveAddr);
  spiSend(&spid, len, reg.raw + static_cast<uint8_t>(idx));
  spiUnselect(&spid);
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
  
  rfm69.reg.opMode_mode = OpMode::FS;
  rfm69.reg.opMode_sequencerOff = 0; // sequencer is activated
  rfm69.reg.opMode_listenOn = 0; // sequencer is activated
  rfm69.reg.datamodul_dataMode = DataMode::CONTINUOUS_NOSYNC;
  rfm69.reg.datamodul_shaping = DataModul::OOK_NOSHAPING;
  rfm69.reg.dioMapping_io3 = 0b01; // TX_READY or RX_READY on DIO3
  rfm69.cacheWrite(Rfm69RegIndex::OpMode,
		   Rfm69RegIndex::AesKey - Rfm69RegIndex::First);
 end:
  return status;
}

float Rfm69OokRadio::getRssi()
{
  systime_t ts = chVTGetSystemTimeX();
  
  rfm69.reg.rssiConfig_start = true;
  rfm69.cacheWrite(Rfm69RegIndex::RssiConfig);
  do {
    rfm69.cacheRead(Rfm69RegIndex::RssiConfig, 2);
  } while ((not rfm69.reg.rssiConfig_done) or
	   (chTimeDiffX(chVTGetSystemTimeX(), ts) < TIME_S2I(1)));

  if (rfm69.reg.rssiConfig_done) 
    return (-rfm69.reg.rssi / 2.0f);
  else
    return -1e6;
}

Rfm69Status Rfm69OokRadio::calibrate()
{
  systime_t ts = chVTGetSystemTimeX();
  
  const auto saveMode = rfm69.reg.opMode_mode;
  rfm69.reg.opMode_mode = OpMode::STDBY;
  rfm69.cacheWrite(Rfm69RegIndex::OpMode);
  
  rfm69.reg.osc1_calibStart = true;
  rfm69.cacheWrite(Rfm69RegIndex::Osc1);
  do {
    rfm69.cacheRead(Rfm69RegIndex::Osc1);
  } while ((not rfm69.reg.osc1_calibDone) or
	   (chTimeDiffX(chVTGetSystemTimeX(), ts) < TIME_S2I(1)));

  rfm69.reg.opMode_mode = saveMode;
  rfm69.cacheWrite(Rfm69RegIndex::Osc1);

  return rfm69.reg.osc1_calibDone ? Rfm69Status::OK : Rfm69Status::TIMOUT;
}

/*
  continous mode without gaussian filter : asynchronous mode without clock

  In OOK, automatic gain control will work if frame begin several all 1
  words : that mean that frame must begin with 4 0xFF before sync beacon

 */
Rfm69Status Rfm69OokRadio::setRfParam(OpMode _mode,
				      uint32_t frequencyCarrier,
				      int8_t amplificationLevelDb)
{
  setFrequencyCarrier(frequencyCarrier);

  mode = _mode;
  GATE::setMode(GATE::MODE::HiZ);
  if (mode == OpMode::TX) {
    GATE::setMode(GATE::MODE::TX);
    setPowerAmp(0b001, RampTime::US_20, amplificationLevelDb);
  } else {
    GATE::setMode(GATE::MODE::RX);
    setReceptionTuning();
  }  
  rfm69.reg.opMode_mode = mode;
  rfm69.cacheWrite(Rfm69RegIndex::OpMode);

  // optional : to be tested, optimisation of floor threshold
  // works only in the absence of module emitting !!
  if (mode == OpMode::RX)
    calibrateRssiThresh();

  const auto status = waitReady();
  DebugTrace("status = %lx", static_cast<uint32_t>(status));
  return status;
}

Rfm69Status Rfm69OokRadio::waitReady(void)
{
  systime_t start = chVTGetSystemTimeX();
  while (chTimeDiffX(start, chVTGetSystemTimeX()) < TIME_MS2I(1000)) {
    rfm69.cacheRead(Rfm69RegIndex::First, sizeof(rfm69.reg.raw));
    if ((mode == OpMode::RX) and rfm69.reg.irqFlags_rxReady)
      return Rfm69Status::OK;
    if ((mode == OpMode::TX) and rfm69.reg.irqFlags_txReady)
      return Rfm69Status::OK;
    chThdSleepMilliseconds(1);
  }
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

void Rfm69OokRadio::setFrequencyCarrier(uint32_t frequencyCarrier)
{
  chDbgAssert((frequencyCarrier > 100'000'000) and (frequencyCarrier < 1'000'000'000),
	      "out of bound frequency carrier");
  rfm69.reg.frf = SWAP_ENDIAN24(frequencyCarrier / synthStepHz);
  rfm69.cacheWrite(Rfm69RegIndex::Frf, 3);

}

void Rfm69OokRadio::setReceptionTuning(void)
{
  
  //rfm69.reg.ookPeak_type = ThresholdType::PEAK;
  rfm69.reg.ookPeak_type = ThresholdType::FIXED;
  rfm69.cacheWrite(Rfm69RegIndex::OokPeak);

  rfm69.reg.ookFix_threshold = 0;
  rfm69.cacheWrite(Rfm69RegIndex::OokFix);
  
  rfm69.reg.afcCtrl_lowBetaOn = false;
  rfm69.cacheWrite(Rfm69RegIndex::AfcCtrl);
  
  rfm69.reg.testDagc = FadingMargin::IMPROVE_LOW_BETA_OFF;
  rfm69.cacheWrite(Rfm69RegIndex::TestDagc);
  
  // settings for RxBw = 20Khz in OOK
  rfm69.reg.rxBw_mant =  BandwithMantissa::MANT_24;
  rfm69.reg.rxBw_exp = 3;
  rfm69.reg.rxBw_dccFreq = 2; // default 4 % of rxbx -> 800hz
  rfm69.cacheWrite(Rfm69RegIndex::RxBw);
  
  // in case of false '1', raise the value to 0xFF 
  rfm69.reg.rssiThresh = 0xE4; // cf datasheet p61
  rfm69.cacheWrite(Rfm69RegIndex::RssiThresh);
  
  // automatic frequency correction activated
  //rfm69.reg.afc_autoOn = true;
  //rfm69.cacheWrite(Rfm69RegIndex::AfcFei);
}


// floor threshold optimisation, see fig 12 of RFM69W datasheet
// 27 step to be tested in maximum one second : 35 ms for each step
void Rfm69OokRadio::calibrateRssiThresh(void)
{
  const float rssi = getRssi();
  DebugTrace("RSSI = %.1f", rssi);
  palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_INPUT);
  auto isDioStableLow = [] {
#ifdef DIO2_DIRECT
    const bool low = palReadLine(LINE_EXTVCP_TX) == PAL_LOW; // no inverting driver
#else
    const bool low = palReadLine(LINE_EXTVCP_TX) == PAL_HIGH; // after the inverting driver
#endif
    DebugTrace("level = %s", low ? "LOW" : "HIGH");
    if (low) {
      const bool stable = palWaitLineTimeout(LINE_EXTVCP_TX, TIME_MS2I(35)) == MSG_TIMEOUT;
      DebugTrace("stable = %s", stable ? "YES" : "NO");
      return stable;
    } else {
      return false;
    }
  };
  
  palEnableLineEvent(LINE_EXTVCP_TX, PAL_EVENT_MODE_BOTH_EDGES);
  for (uint16_t t = 0xE4; t <= 0xFF; t++) {
    rfm69.reg.rssiThresh = t;
    DebugTrace("rfm69.reg.rssiThresh = %d", rfm69.reg.rssiThresh);
    rfm69.cacheWrite(Rfm69RegIndex::RssiThresh);
    if (isDioStableLow())
      break;
  }
  palDisableLineEvent(LINE_EXTVCP_TX);
  palSetLineMode(LINE_EXTVCP_TX, PAL_MODE_ALTERNATE(AF_LINE_EXTVCP_TX));

  // rfm69.reg.ookPeak_threshDec = ThresholdDec::EIGHT_TIMES;
  // rfm69.reg.ookPeak_threshStep = ThresholdStep::DB_0P5;
  // rfm69.cacheWrite(Rfm69RegIndex::OokPeak);
}
