#pragma once
#include "operations.hpp"
#include "etl/string.h"
#include "etl/string_view.h"
#include "common.hpp"
#include "hardwareConf.hpp"

class BBoard
{
public:
  using error_t = etl::string<16>;
  using source_t = etl::string<10>;
  using dio2Threshold_t = std::pair<float, float>;
  void setMode(Ope::Mode _mode) {mode = _mode;}
  void setRssi(int16_t _rssi) {rssi = _rssi;}
  void setLnaGain(int16_t _lnaGain) {lnaGain = _lnaGain;}
  void setBer(uint16_t _ber) {ber = _ber;}
  void setDioAvg(float _dioAvg) {dioAvg = _dioAvg;}
  void setBitRateIdx(BitRateIndex _baudIdx) {baudIdx = _baudIdx;}
  void setFreq(uint32_t _freq) {freq = _freq;}
  void setRfEnable(bool _rfEnable) {rfEnable = _rfEnable;}
  void setTxPower(int16_t _txPower) {txPower = _txPower;}
  void setDio2Threshold(dio2Threshold_t dt) {dio2Threshold = dt;}
  void setError(etl::string_view _error) {Lock m(mtx);
    error =  error_t{_error};}
  void clearError(void) {Lock m(mtx); error.clear();}
  void setSource(etl::string_view _source) {Lock m(mtx);
    source =  source_t{_source};}

  Ope::Mode getMode(void) {return mode;}
  int16_t getRssi(void) {return rssi;}
  int16_t getLnaGain(void) {return lnaGain;}
  uint16_t getBer(void) {return ber;}
  float   getDioAvg(void) {return dioAvg;}
  BitRateIndex getBitRateIdx(void) {return baudIdx;}
  uint32_t getBaud(void) {return baudRates[+baudIdx];}
  uint32_t getFreq(void) {return freq;}
  bool getRfEnable(void) {return rfEnable;}
  int16_t getTxPower(void) {return txPower;}
  dio2Threshold_t getDio2Threshold(void) {return dio2Threshold;}
  const error_t& getError(void) {Lock m(mtx); return error;}
  const source_t& getSource(void) {Lock m(mtx); return source;}
  
private:
  // scalar assignment is atomic on arm32 arch, so mutex
  // is only need to protect string error data.
  MUTEX_DECL(mtx);
  Ope::Mode mode = Ope::Mode::NONE;
  int16_t rssi = {};
  int16_t lnaGain = {};
  uint16_t ber = {};
  float   dioAvg = {};
  BitRateIndex baudIdx = {};
  uint32_t freq = {};
  bool    rfEnable = false;
  int16_t txPower = {};
  error_t error = {};
  source_t source = {};
  dio2Threshold_t dio2Threshold = {0, 1};
};

extern BBoard board;
