#pragma once

#include <ch.h>
#include <hal.h>
#include <math.h>
#include "rfm69_registers.hpp"

/*
 
 */

enum class Rfm69Status {OK, INIT_ERROR, INTERNAL_ERROR, TIMOUT};

class Rfm69Spi {
public:
  Rfm69Spi(SPIDriver& _spid, ioline_t _lineReset) : spid(_spid), lineReset(_lineReset) {};
  Rfm69Status init(const SPIConfig& spiCfg);
  bool isInit(void) {return spid.state >= SPI_READY;}
  void cacheWrite(Rfm69RegIndex idx, size_t len = 1);
  void cacheRead(Rfm69RegIndex idx, size_t len = 1);
  void reset(void);

  volatile Rfm69Rmap reg {}; // registers are public to avoid massive
  // setter getters code with the numerous bitfields
  Rfm69Rmap regSave {};
private:
  SPIDriver& spid;
  ioline_t  lineReset;
  void saveReg(void);
  void restoreReg(void);
};

#define GET_DECL(name, bft, bfn, regi)		\
  bft get##name(void) \
  { \
    rfm69.cacheRead(Rfm69RegIndex::regi, 1U); \
    return rfm69.reg.bfn; \
  }

#define SET_DECL(name, bft, bfn, regi)	\
  void set##name(bft v) \
  { \
    rfm69.reg.bfn = v; \
    rfm69.cacheWrite(Rfm69RegIndex::regi, 1U); \
  }

#define GSET_DECL(name, bft, bfn, regi)	\
  GET_DECL(name, bft, bfn, regi) \
  SET_DECL(name, bft, bfn, regi)

class Rfm69OokRadio {
  static constexpr uint32_t xtalHz = 32e6;
  static constexpr float synthStepHz = xtalHz / powf(2,19);

public:
  
public:
  Rfm69OokRadio(SPIDriver& spid, ioline_t lineReset) :
    rfm69(spid, lineReset) {};
  Rfm69Status init(const SPIConfig& spiCfg);
  bool isInit(void) {return rfm69.isInit();}
  Rfm69Status setRfParam(RfMode _mode, 
			 uint32_t frequencyCarrier,
			 int8_t amplificationLevelDb);
  void	setBaudRate(uint32_t br);
  Rfm69Status waitReady(void);
  Rfm69Status calibrate(void);
  void coldReset();
  void checkModeMismatch(void);
  void checkRestartRxNeeded(void);
  float getRssi();
  int8_t getLnaGain(void);
  void humanDisplayFlags(void);
  RfMode getOrderMode() {return mode;}
  GSET_DECL(RfMode, RfMode, opMode_mode, RfMode);
  GET_DECL(RxReady, bool, irqFlags_rxReady, IrqFlags1);
  SET_DECL(Afc_force, bool, afc_start, AfcFei);
  SET_DECL(RestartRx, bool, packetConfig2_restartRx, PacketConfig2);
  SET_DECL(AutoRxRestart, bool, packetConfig2_autoRxRestartOn, PacketConfig2);
  Rfm69Status setModeAndWait(RfMode mode);
protected:
  Rfm69Spi rfm69;
  RfMode mode {RfMode::SLEEP};
  thread_t *rfHealthSurveyThd = nullptr;
  static THD_WORKING_AREA(waSurvey, 512);
  
  GSET_DECL(Dagc, FadingMargin, testDagc, TestDagc);
  GSET_DECL(LowBetaOn, bool, afcCtrl_lowBetaOn, AfcCtrl);
  GSET_DECL(OokFix_threshold, uint8_t, ookFix_threshold, OokFix);
  GSET_DECL(Rssi_threshold, uint8_t, rssiThresh, RssiThresh);
  GSET_DECL(Afc_autoOn, bool, afc_autoOn, AfcFei);
  GSET_DECL(Ocp_on, bool, ocp_on, Ocp);

  void calibrateRssiThresh(void);
  void setFrequencyCarrier(uint32_t frequencyCarrier);
  void setPowerAmp(uint8_t pmask, RampTime rt, int8_t gain);
  void setLna(LnaGain gain, LnaInputImpedance imp);
  void setReceptionTuning(void);
  void setOokPeak(ThresholdType t, ThresholdDec d, ThresholdStep s);
  void setRxBw(BandwithMantissa, uint8_t exp, uint8_t dccFreq);
  static void rfHealthSurvey(void *arg);
};

