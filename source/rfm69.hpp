#pragma once

#include <ch.h>
#include <hal.h>
#include <math.h>
#include "rfm69_registers.hpp"

/*
 
 */

enum class Rfm69Status {OK, INIT_ERROR, TIMOUT};


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


 



class Rfm69OokRadio {
  static constexpr uint32_t xtalHz = 32e6;
  static constexpr float synthStepHz = xtalHz / powf(2,19);

public:
  
public:
  Rfm69OokRadio(SPIDriver& spid, ioline_t lineReset) :
    rfm69(spid, lineReset) {};
  Rfm69Status init(const SPIConfig& spiCfg);
  Rfm69Status setRfParam(OpMode _mode, uint32_t frequencyCarrier,
			 int8_t amplificationLevelDb);
  Rfm69Status waitReady(void);
  Rfm69Status calibrate(void);
  float getRssi();
protected:
  Rfm69Spi rfm69;
  OpMode mode {OpMode::SLEEP};

  void calibrateRssiThresh(void);
  void setFrequencyCarrier(uint32_t frequencyCarrier);
  void setPowerAmp(uint8_t pmask, RampTime rt, int8_t gain);
  void setReceptionTuning(void);
};
