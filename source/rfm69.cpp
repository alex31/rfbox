#include "rfm69.hpp"
#include "stdutil.h"
#include "operations.hpp"
#include "hardwareConf.hpp"
#include "dio2Spy.hpp"
#include "etl/string.h"
#include "bboard.hpp"
#include "common.hpp"

#define SWAP_ENDIAN24(x) __builtin_bswap32(static_cast<uint32_t>(x) << 8)

namespace {
  constexpr uint8_t readMask =  0x0;
  constexpr uint8_t writeMask = 0x80;
  MUTEX_DECL(protectMtx);
#if PARANOID_REGREAD
  Rfm69Rmap regCheck;
#endif
}

#define NOREENT()   Lock m(protectMtx) 

/*
#                 ______   _ __    _          
#                /  ____| | '_ \  (_)         
#                | (___   | |_) |  _          
#                 \___ \  | .__/  | |         
#                .____) | | |     | |         
#                \_____/  |_|     |_|         
*/


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
#if PARANOID_REGREAD
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
#else
    spiSelect(&spid);
    const uint8_t slaveAddr = static_cast<uint8_t>(idx) | readMask;
    spiSend(&spid, 1, &slaveAddr);
    spiReceive(&spid, len, const_cast<uint8_t *>(reg.raw) +
	       static_cast<uint32_t>(idx));
    spiUnselect(&spid);
#endif
   // chThdSleepMicroseconds(10);
  spiReleaseBus(&spid);
  // chThdSleepMicroseconds(10);
}

void Rfm69Spi::cacheWrite(Rfm69RegIndex idx, size_t len)
{
  spiAcquireBus(&spid);
  spiSelect(&spid);
  const uint8_t slaveAddr = static_cast<uint8_t>(idx) | writeMask;
  spiSend(&spid, 1, &slaveAddr);
  spiSend(&spid, len, const_cast<uint8_t *>(reg.raw) + static_cast<uint32_t>(idx));
  spiUnselect(&spid);
  spiReleaseBus(&spid);
}

void Rfm69Spi::fifoRead(void *buffer, const uint8_t len)
{
  spiAcquireBus(&spid);
  spiSelect(&spid);
  const uint8_t slaveAddr = 0U | readMask;
  spiSend(&spid, 1, &slaveAddr);
  spiReceive(&spid, len, static_cast<uint8_t *>(buffer));
  spiUnselect(&spid);
  
  spiReleaseBus(&spid);
}

void Rfm69Spi::fifoWrite(const void *buffer, const uint8_t len)
{
  spiAcquireBus(&spid);
  spiSelect(&spid);
  const uint8_t slaveAddr = 0U | writeMask;
  spiSend(&spid, 1, &slaveAddr);
  spiSend(&spid, len, static_cast<const uint8_t *>(buffer));
  spiUnselect(&spid);
  spiReleaseBus(&spid);
}


/*
#                 ____                          _____                _    _                  
#                |  _ \                        |  __ \              | |  (_)                 
#                | |_) |   __ _   ___     ___  | |__) |   __ _    __| |   _     ___          
#                |  _ <   / _` | / __|   / _ \ |  _  /   / _` |  / _` |  | |   / _ \         
#                | |_) | | (_| | \__ \  |  __/ | | \ \  | (_| | | (_| |  | |  | (_) |        
#                |____/   \__,_| |___/   \___| |_|  \_\  \__,_|  \__,_|  |_|   \___/         
*/


float Rfm69BaseRadio::getRssi()
{
  rfm69.cacheRead(Rfm69RegIndex::RssiConfig, 2);
  const float rssi = -rfm69.reg.rssi / 2.0f;
  board.setRssi(rssi);
  return rssi;
}

void	 Rfm69BaseRadio::setBaudRate(uint32_t br)
{
  const uint16_t bitrate = xtalHz / br;
  rfm69.reg.bitrate = SWAP_ENDIAN16(bitrate);
  rfm69.cacheWrite(Rfm69RegIndex::Bitrate);
}


void Rfm69BaseRadio::setCommonRfParam(uint32_t frequencyCarrier,
					     int8_t amplificationLevelDb
					     )
{
  setFrequencyCarrier(frequencyCarrier);
  setPowerAmp(0b001, RampTime::US_20, amplificationLevelDb);
  setRfTuning();
  setLna(LnaGain::AGC, LnaInputImpedance::OHMS_50);
}

Rfm69Status Rfm69BaseRadio::healthSurveyStart(RfMode _mode)
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


  auto status = setModeAndWait(mode);
  DebugTrace("wait status for mode[%lx] = %lx",
	     static_cast<uint32_t>(mode),
	     static_cast<uint32_t>(status));
  if (status != Rfm69Status::OK)
    goto exit;
  if (mode == RfMode::RX) {
    DebugTrace("current lna gain = %d ... RSSI = %.1f",
	       getLnaGain(), getRssi());
  }

  if ((mode == RfMode::RX) or (mode == RfMode::TX)) 
    rfHealthSurveyThd = chThdCreateStatic(waSurvey, sizeof(waSurvey),
					 NORMALPRIO - 1, &rfHealthSurvey, this);
  
 exit:
  return status;
}



Rfm69Status Rfm69BaseRadio::calibrate()
{
  NOREENT();
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

  return rfm69.reg.osc1_calibDone ? Rfm69Status::OK : Rfm69Status::TIMOUT;
}

Rfm69Status Rfm69BaseRadio::waitReady(void)
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



void Rfm69BaseRadio::setPowerAmp(uint8_t pmask, RampTime rt, int8_t gain)
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


void Rfm69BaseRadio::setRxBw(BandwithMantissa bm, uint8_t exp,
			    uint8_t dccFreq)
{
  rfm69.reg.rxBw_mant =  bm;
  rfm69.reg.rxBw_exp = exp;
  rfm69.reg.rxBw_dccFreq = dccFreq; // default 4 % of rxbw
  rfm69.cacheWrite(Rfm69RegIndex::RxBw);
}

void Rfm69BaseRadio::setFrequencyCarrier(uint32_t frequencyCarrier)
{
  chDbgAssert((frequencyCarrier > 100'000'000) and (frequencyCarrier < 1'000'000'000),
	      "out of bound frequency carrier");
  rfm69.reg.frf = SWAP_ENDIAN24(frequencyCarrier / synthStepHz);
  rfm69.cacheWrite(Rfm69RegIndex::Frf, 3);
}


void Rfm69BaseRadio::setLna(LnaGain gain, LnaInputImpedance imp)
{
  rfm69.reg.lna_gain = gain;
  rfm69.reg.lna_zIn  = imp;
  rfm69.cacheWrite(Rfm69RegIndex::Lna);
}

int8_t Rfm69BaseRadio::getLnaGain(void)
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


void Rfm69BaseRadio::humanDisplayFlags(void)
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


Rfm69Status Rfm69BaseRadio::setModeAndWait(RfMode nmode)
{
  setRfMode(nmode);
  return waitReady();
}

void Rfm69BaseRadio::rfHealthSurvey(void *arg)
{
  Rfm69BaseRadio *radio = static_cast<Rfm69BaseRadio *>(arg);
  chRegSetThreadName("RF Health survey");
  uint32_t calCount = 0, cmmCount = 0, restartRxCount = 0;;
  
  while (not chThdShouldTerminateX()) {
    chThdSleepMilliseconds(100);
   if (++calCount > 600) {
      radio->calibrate();
      calCount = 0;
    } else if (++restartRxCount > 15) {
      radio->checkRestartRxNeeded();
      restartRxCount = 0;
    } else if (++cmmCount > 10) {
      radio->checkModeMismatch();
      cmmCount = 0;
    }
  }
  chThdExit(MSG_OK);
}

void Rfm69BaseRadio::coldReset()
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


THD_WORKING_AREA(Rfm69BaseRadio::waSurvey, 512);

/*
#                  ____            _      _____                _    _                  
#                 / __ \          | |    |  __ \              | |  (_)                 
#                | |  | |   ___   | | _  | |__) |   __ _    __| |   _     ___          
#                | |  | |  / _ \  | |/ / |  _  /   / _` |  / _` |  | |   / _ \         
#                | |__| | | (_) | |   <  | | \ \  | (_| | | (_| |  | |  | (_) |        
#                 \____/   \___/  |_|\_\ |_|  \_\  \__,_|  \__,_|  |_|   \___/         
*/


Rfm69Status Rfm69OokRadio::init(const SPIConfig& spiCfg)
{
  Rfm69Status status = rfm69.init(spiCfg);
  if (status != Rfm69Status::OK)
    goto end;

  if ((status = calibrate()) != Rfm69Status::OK)
    goto end;

  setCommonRfParam(board.getFreq(), board.getTxPower());
  rfm69.reg.opMode_mode = mode; // mode or RfMode::FS
  rfm69.reg.opMode_sequencerOff = 0; // sequencer is activated
  rfm69.reg.opMode_listenOn = 0; 
  rfm69.reg.datamodul_dataMode = DataMode::CONTINUOUS_NOSYNC;
  rfm69.reg.bitrate = SWAP_ENDIAN16(xtalHz / board.getBaud());
  rfm69.reg.datamodul_shaping = DataModul::OOK_NOSHAPING;
  rfm69.cacheWrite(Rfm69RegIndex::DataModul,
		   Rfm69RegIndex::AesKey - Rfm69RegIndex::DataModul);
  setBaudRate(board.getBaud());
  rfm69.cacheWrite(Rfm69RegIndex::RfMode);
  waitReady();
 end:
  return status;
}




void Rfm69OokRadio::checkModeMismatch()
{
  NOREENT();
  rfm69.cacheRead(Rfm69RegIndex::RfMode);
  if ((mode != RfMode::SLEEP) and (rfm69.reg.opMode_mode != mode)) {
    DebugTrace("mismatch found mode %u instead %u, reset...",
	       static_cast<uint16_t>(rfm69.reg.opMode_mode),
	       static_cast<uint16_t>(mode));
    board.setError("Radio lockout");
    coldReset();
  } else if (const float av = Dio2Spy::getAverageLevel();
	     (mode == RfMode::RX) and ((av < 0.01) or (av > 0.70f))) {
    DebugTrace("dio2 average not valid = %.2f", av);
    board.setError("DIO2 Avg Err");
    coldReset();
    setRestartRx(true);
  } else {
    board.clearError();
  }
}

void Rfm69OokRadio::checkRestartRxNeeded()
{
  NOREENT();
  const float rssi = getRssi();
  const float lnaGain = getLnaGain();
  //  static systime_t ts = 0;
  if ( ((rssi < -60.0f ) and (lnaGain < -24.0f))
       or
       ((rssi > -60.0f) and (lnaGain > -24.0f))
       //      or
       //       (not chVTIsSystemTimeWithin(ts, chTimeAddX(ts, TIME_S2I(1000))))
       ) {
    DebugTrace("RSSI LNA Gain setRestartRx condition");
    setRestartRx(true);
    //    ts = chVTGetSystemTime();
  }
}



void Rfm69OokRadio::forceRestartRx()
{
  NOREENT();
  setRestartRx(true);
}

// floor threshold optimisation, see fig 12 of RFM69W datasheet
// 27 step to be tested in maximum one second : 35 ms for each step
void Rfm69OokRadio::calibrateRssiThresh(void)
{
  auto isDioStableLow = [] {
    const bool low = palReadLine(LINE_EXTVCP_TX) == PAL_LOW; 
    DebugTrace("level = %s", low ? "LOW" : "HIGH");
    if (low) {
      const bool stable = palWaitLineTimeout(LINE_EXTVCP_TX, TIME_MS2I(35)) == MSG_TIMEOUT;
      DebugTrace("stable = %s", stable ? "YES" : "NO");
      return stable;
    } else {
      return false;
    }
  };

  Ope::setMode(Ope::Mode::RF_CALIBRATE_RSSI);
  
  palEnableLineEvent(LINE_EXTVCP_TX, PAL_EVENT_MODE_BOTH_EDGES);
  for (uint16_t t = 0xB0; t <= 0xFF; t++) {
    rfm69.reg.rssiThresh = t;
    DebugTrace("rfm69.reg.rssiThresh = %d", rfm69.reg.rssiThresh);
    rfm69.cacheWrite(Rfm69RegIndex::RssiThresh);
    if (isDioStableLow())
      break;
  }
  palDisableLineEvent(LINE_EXTVCP_TX);

  // rfm69.reg.ookPeak_threshDec = ThresholdDec::EIGHT_TIMES;
  // rfm69.reg.ookPeak_threshStep = ThresholdStep::DB_0P5;
  // rfm69.cacheWrite(Rfm69RegIndex::OokPeak);
}

void Rfm69OokRadio::setRfTuning(void)
{
  setOokPeak(ThresholdType::PEAK, ThresholdDec::EIGHT_TIMES,
	     ThresholdStep::DB_3);

  setLowBetaOn(true);
  setDagc(FadingMargin::IMPROVE_LOW_BETA_ON);
  setAutoRxRestart(false);
  
  // settings for RxBw = 20Khz in OOK
  if (board.getBaud() == baudLow)
    setRxBw(BandwithMantissa::MANT_24, 4, 2); /* 10 Khz bandwith, dccfreq default 4 % of rxbx */
  else
    setRxBw(BandwithMantissa::MANT_24, 2, 2); /* 42Khz bandwith, dccfreq default 4 % of rxbx */
  
  // in case of false '1', raise the value to 0xFF cf datasheet p61
  setRssi_threshold(0xE4);
  DebugTrace("rfm69.reg.rssiThresh = %d", rfm69.reg.rssiThresh);
  //  calibrateRssiThresh();
  // automatic frequency correction activated
  setAfc_autoOn(true);

  // overcurrent protection to limit current consomption : we don't care
  setOcp_on(false);
}

    


void Rfm69OokRadio::setOokPeak(ThresholdType t, ThresholdDec d,
			       ThresholdStep s)
{
  rfm69.reg.ookPeak_type = t;
  rfm69.reg.ookPeak_threshDec = d;
  rfm69.reg.ookPeak_threshStep = s;
  rfm69.cacheWrite(Rfm69RegIndex::OokPeak);
}






/*
#                 ______          _      _____                _    _                  
#                |  ____|        | |    |  __ \              | |  (_)                 
#                | |__     ___   | | _  | |__) |   __ _    __| |   _     ___          
#                |  __|   / __|  | |/ / |  _  /   / _` |  / _` |  | |   / _ \         
#                | |      \__ \  |   <  | | \ \  | (_| | | (_| |  | |  | (_) |        
#                |_|      |___/  |_|\_\ |_|  \_\  \__,_|  \__,_|  |_|   \___/         
*/
Rfm69Status Rfm69FskRadio::init(const SPIConfig& spiCfg)
{
  Rfm69Status status = rfm69.init(spiCfg);
  if (status != Rfm69Status::OK)
    goto end;

  if ((status = calibrate()) != Rfm69Status::OK)
    goto end;

  // in packet mode, we want rf transfert to be faster than wire transfert
  // to avoid congestion
  setCommonRfParam(board.getFreq(), board.getTxPower());
  setBaudRate(board.getBaud() * 2U);
  rfm69.reg.opMode_mode = mode; // mode or RfMode::FS
  rfm69.reg.opMode_sequencerOff = 0; // sequencer is activated
  rfm69.reg.opMode_listenOn = 0; 
  rfm69.reg.datamodul_dataMode = DataMode::PACKET;
  rfm69.reg.bitrate = SWAP_ENDIAN16(xtalHz / board.getBaud());
  rfm69.reg.datamodul_shaping = DataModul::FSK_NOSHAPING;
  rfm69.reg.dioMapping_io1 = 0b11;
  rfm69.reg.dioMapping_io0 = 
    rfm69.reg.dioMapping_io2 = 
    rfm69.reg.dioMapping_io3 = 
    rfm69.reg.dioMapping_io4 = 
    rfm69.reg.dioMapping_io5 = 0b10; // DIO2 is HiZ in Tx and Rx modes and won't mess with UART_TX

  configPacketMode();
  setFrequencyDeviation(frequencyDev); // roughly 3x the bitrate
  
  rfm69.cacheWrite(Rfm69RegIndex::DataModul,
		   Rfm69RegIndex::AesKey - Rfm69RegIndex::DataModul);
  rfm69.cacheWrite(Rfm69RegIndex::RfMode);
  waitReady();
 end:
  return status;
}


void Rfm69FskRadio::checkModeMismatch()
{
  NOREENT();
  rfm69.cacheRead(Rfm69RegIndex::RfMode);
  if ((mode != RfMode::SLEEP) and (rfm69.reg.opMode_mode != mode)) {
    DebugTrace("mismatch found mode %u instead %u, reset...",
	       static_cast<uint16_t>(rfm69.reg.opMode_mode),
	       static_cast<uint16_t>(mode));
    board.setError("Radio lockout");
    coldReset();
  } else {
    board.clearError();
  }
}


void  Rfm69FskRadio::configPacketMode(void)
{
  rfm69.reg.preambleSize = SWAP_ENDIAN16(preembleSize);
  rfm69.reg.syncConfig_tol = 0;
  rfm69.reg.syncConfig_size = syncWordSize - 1U;
  rfm69.reg.syncConfig_fifoFillCondition = true;
  rfm69.reg.syncConfig_syncOn = true;
  rfm69.reg.syncValue = 0xFEEDFEEDFEEDFEED;
  rfm69.reg.packetConfig_addressFiltering = AddressFiltering::NONE;
  rfm69.reg.packetConfig_crcOn = false;
  rfm69.reg.packetConfig_dcFree = DCFree::WHITENING;
  rfm69.reg.packetConfig_packetFormat = PacketFormat::VARIABLE_LEN;
  rfm69.reg.payloadLength = fifoMaxLen + 1U;
  rfm69.reg.autoModes_enterCondition = EnterCondition::NONE;
  rfm69.reg.autoModes_exitCondition = ExitCondition::NONE;
  rfm69.reg.fifoThresh_txStartCondition = TxStartCondition::FIFO_NOT_EMPTY;
  rfm69.reg.packetConfig2_interPacketRxDelay = interPacketRxDelay;
  rfm69.reg.packetConfig2_autoRxRestartOn = true;
  rfm69.reg.packetConfig2_aesOn = false;
}

void Rfm69FskRadio::setFrequencyDeviation(uint32_t frequencyDeviation)
{
  const uint32_t fdev = frequencyDeviation / synthStepHz;
  chDbgAssert((fdev > 20U) and (fdev < (1U << 14U)),
	      "out of bound deviation frequency");
  
  rfm69.reg.fdev = SWAP_ENDIAN16(fdev);
}

void Rfm69FskRadio::fifoWrite(const void *buffer, const uint8_t len)
{
  rfm69.fifoWrite(buffer, len);
}

void Rfm69FskRadio::fifoRead(void *buffer, uint8_t *len)
{
  rfm69.fifoRead(len, 1);
  if (*len != 0U) 
    rfm69.fifoRead(buffer, *len);
}

void Rfm69FskRadio::setRfTuning(void)
{
}
