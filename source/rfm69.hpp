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

  void cacheWrite(Rfm69RegIndex idx, size_t len = 1);
  void cacheRead(Rfm69RegIndex idx, size_t len = 1);

  Rfm69Rmap reg; // registers are public to avoid massive
  // setter getters code with the numerous bitfields
  
private:
  void reset(void);
  SPIDriver& spid;
  ioline_t  lineReset;
};


 
#define GSET_DECL(name, bft, bfn, regi)	\
  void set##name(bft v) \
  { \
    rfm69.reg.bfn = v; \
    rfm69.cacheWrite(Rfm69RegIndex::regi, 1U); \
  } \
  bft get##name(void) \
  { \
    rfm69.cacheRead(Rfm69RegIndex::regi, 1U); \
    return rfm69.reg.bfn; \
  } \


class Rfm69OokRadio {
  static constexpr uint32_t xtalHz = 32e6;
  static constexpr float synthStepHz = xtalHz / powf(2,19);

public:
  
public:
  Rfm69OokRadio(SPIDriver& spid, ioline_t lineReset) :
    rfm69(spid, lineReset) {};
  Rfm69Status init(const SPIConfig& spiCfg);
  Rfm69Status setRfParam(RfMode _mode, 
			 uint32_t frequencyCarrier,
			 int8_t amplificationLevelDb);
  Rfm69Status waitReady(void);
  Rfm69Status calibrate(void);
  float getRssi();
  RfMode getMode() {return mode;}

protected:
  Rfm69Spi rfm69;
  RfMode mode {RfMode::SLEEP};
  
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
  int8_t getLnaGain(void);
  void setReceptionTuning(void);
  void setOokPeak(ThresholdType t, ThresholdDec d, ThresholdStep s);
  void setRxBw(BandwithMantissa, uint8_t exp, uint8_t dccFreq);
};

