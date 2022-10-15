#pragma once

#include <ch.h>
#include <hal.h>
#include <math.h>
#include "rfm69_registers.hpp"
#include "hardwareConf.hpp"

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
  void fifoWrite(const void *buffer, const uint8_t len);
  void fifoRead(void *buffer, const uint8_t len);
  void reset(void);

  volatile Rfm69Rmap reg {}; // registers are public to avoid massive
  // setter getters code with the numerous bitfields

private:
  SPIDriver& spid;
  ioline_t  lineReset;
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

class Rfm69BaseRadio {
public:
  Rfm69BaseRadio(SPIDriver& spid, ioline_t lineReset) :
    rfm69(spid, lineReset) {};
  virtual Rfm69Status init(const SPIConfig& spiCfg) = 0;
  bool isInit(void) {return rfm69.isInit();}
  Rfm69Status healthSurveyStart(RfMode _mode);
  void setCommonRfParam(uint32_t frequencyCarrier,
			int8_t amplificationLevelDb);
  void	setBitRate(uint32_t br);
  Rfm69Status waitReady(void);
  Rfm69Status calibrate(void);
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
  virtual void forceRestartRx() {};
  virtual void checkModeMismatch(void) = 0;
  virtual void checkRestartRxNeeded(void) {};
  void coldReset();

protected:
  GSET_DECL(LowBetaOn, bool, afcCtrl_lowBetaOn, AfcCtrl);
  GSET_DECL(Dagc, FadingMargin, testDagc, TestDagc);
  GSET_DECL(Rssi_threshold, uint8_t, rssiThresh, RssiThresh);
  GSET_DECL(Afc_autoOn, bool, afc_autoOn, AfcFei);
  GSET_DECL(Ocp_on, bool, ocp_on, Ocp);
  static constexpr uint32_t xtalHz = 32e6;
  static constexpr uint32_t fxOscHz = xtalHz;
  static constexpr float synthStepHz = xtalHz / powf(2,19);
  
  thread_t *rfHealthSurveyThd = nullptr;
  static THD_WORKING_AREA(waSurvey, 512);
  static void rfHealthSurvey(void *arg);

  Rfm69Spi rfm69;
  RfMode mode {RfMode::SLEEP};
  
 
  virtual void setRfTuning(void) = 0;
  void setFrequencyCarrier(uint32_t frequencyCarrier);
  void setPowerAmp(uint8_t pmask, RampTime rt, int8_t gain);
  void setLna(LnaGain gain, LnaInputImpedance imp);
  void setRxBw(BandwithMantissa, uint8_t exp, uint8_t dccFreq);
  void setAfcBw(BandwithMantissa, uint8_t exp, uint8_t dccFreq);

};

class Rfm69OokRadio : public Rfm69BaseRadio {
public:
  Rfm69OokRadio(SPIDriver& spid, ioline_t lineReset) :
    Rfm69BaseRadio(spid, lineReset) {};
  Rfm69Status init(const SPIConfig& spiCfg) override;

  void checkModeMismatch(void) override;
  void checkRestartRxNeeded(void) override;
  void forceRestartRx() override;

protected:
  GSET_DECL(OokFix_threshold, uint8_t, ookFix_threshold, OokFix);

  void calibrateRssiThresh(void);
  void setRfTuning(void) override;
  void setOokPeak(ThresholdType t, ThresholdDec d, ThresholdStep s);
  
  
private:
  ~Rfm69OokRadio() = delete;
};


struct FrequencyDev {
  uint32_t low;
  uint32_t high;
  uint32_t veryHigh;
} ;

class Rfm69FskRadio : public Rfm69BaseRadio {
public:
  Rfm69FskRadio(SPIDriver& spid, ioline_t lineReset) :
    Rfm69BaseRadio(spid, lineReset) {};
  Rfm69Status init(const SPIConfig& spiCfg) override;
  void checkModeMismatch(void) override;
  void fifoWrite(const void *buffer, const uint8_t len);
  void fifoRead(void *buffer, uint8_t *len);
  void rawFifoRead(void *buffer, uint8_t len);

  GET_DECL(PayloadReady, bool, irqFlags_payloadReady, IrqFlags2);
  GET_DECL(PacketSent, bool, irqFlags_packetSent, IrqFlags2);
  GET_DECL(FifoOverrun, bool, irqFlags_fifoOverrun, IrqFlags2);
  GET_DECL(FifoNotEmpty, bool, irqFlags_fifoNotEmpty, IrqFlags2);
  GET_DECL(FifoFull, bool, irqFlags_fifoFull, IrqFlags2);
  GET_DECL(FifoLevel, bool, irqFlags_fifoLevel, IrqFlags2);
protected:
  void setRfTuning(void) override;
  void setFrequencyDeviation(uint32_t frequencyDeviation);
  
private:
  static constexpr uint8_t fifoMaxLen = 66U;
  static constexpr uint8_t preambleSize = 8U;
  static constexpr uint8_t syncWordSize = 4U;
  static constexpr uint8_t interPacketRxDelay = 15U;
  
  void configPacketMode(void);
  ~Rfm69FskRadio() = delete;
};
