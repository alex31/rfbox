#pragma once
#include "operations.hpp"
#include "etl/string.h"
#include "etl/string_view.h"

class BBoard
{
public:
  using error_t = etl::string<16>;
  void setMode(Ope::Mode _mode) {mode = _mode;}
  void setRssi(int16_t _rssi) {rssi = _rssi;}
  void setLnaGain(int16_t _lnaGain) {lnaGain = _lnaGain;}
  void setBer(int16_t _ber) {ber = _ber;}
  void setBaud(int16_t _baud) {baud = _baud;}
  void setFreq(uint32_t _freq) {freq = _freq;}
  void setRfEnable(bool _rfEnable) {rfEnable = _rfEnable;}
  void setTxPower(int16_t _txPower) {txPower = _txPower;}
  void setError(etl::string_view _error) {error =  error_t{_error};}
  void clearError(void) {error.clear();}

  Ope::Mode getMode(void) {return mode;}
  int16_t getRssi(void) {return rssi;}
  int16_t getLnaGain(void) {return lnaGain;}
  int16_t getBer(void) {return ber;}
  int16_t getBaud(void) {return baud;}
  uint32_t getFreq(void) {return freq;}
  bool getRfEnable(void) {return rfEnable;}
  int16_t getTxPower(void) {return txPower;}
  const error_t& getError(void) {return error;}
  
private:
  Ope::Mode mode = Ope::Mode::NONE;
  int16_t rssi = {};
  int16_t lnaGain = {};
  int16_t ber = {};
  int16_t baud = {};
  int32_t freq = {};
  bool    rfEnable = false;
  uint8_t txPower = {};
  error_t error = {};
};

extern BBoard board;
