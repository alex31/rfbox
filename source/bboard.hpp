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
  void setBer(uint16_t _ber) {ber = _ber;}
  void setDioAvg(float _dioAvg) {dioAvg = _dioAvg;}
  void setBaud(uint16_t _baud) {baud = _baud;}
  void setFreq(uint32_t _freq) {freq = _freq;}
  void setRfEnable(bool _rfEnable) {rfEnable = _rfEnable;}
  void setTxPower(int16_t _txPower) {txPower = _txPower;}
  void setError(etl::string_view _error) {Lock m(mtx);
    error =  error_t{_error};}
  void clearError(void) {Lock m(mtx); error.clear();}

  Ope::Mode getMode(void) {return mode;}
  int16_t getRssi(void) {return rssi;}
  int16_t getLnaGain(void) {return lnaGain;}
  uint16_t getBer(void) {return ber;}
  float   getDioAvg(void) {return dioAvg;}
  uint16_t getBaud(void) {return baud;}
  uint32_t getFreq(void) {return freq;}
  bool getRfEnable(void) {return rfEnable;}
  int16_t getTxPower(void) {return txPower;}
  const error_t& getError(void) {Lock m(mtx); return error;}
  
private:
  // scalar assignment is atomic on arm32 arch, so mutex
  // is only need to protect string error data.
  class Lock
  {
  public:
    Lock(mutex_t &_mtx) : mtx(_mtx) {chMtxLock(&mtx);};
    ~Lock() {chMtxUnlock(&mtx);};
  private:
    mutex_t &mtx;
  };
  MUTEX_DECL(mtx);
  Ope::Mode mode = Ope::Mode::NONE;
  int16_t rssi = {};
  int16_t lnaGain = {};
  uint16_t ber = {};
  float   dioAvg = {};
  uint16_t baud = {};
  uint32_t freq = {};
  bool    rfEnable = false;
  uint8_t txPower = {};
  error_t error = {};
};

extern BBoard board;
