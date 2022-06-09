#include "tda5150.hpp"
#include "stdutil.h"
#include "spi_lld_extra.h"
#include <array>

void Tda5150::initSpi()
{
  unselect();
  spiStart(&spid, &spicfg);
  tiedTxMosi.select(lineMosi);
#if TIED_CLOCK
  tiedCkClk.select(lineClk);
#endif
}


void Tda5150::writeSfr(const std::initializer_list<AddrVal>& values)
{
  for (const auto [a, v] : values) {
    writeSfr(a, v);
  }
}

void Tda5150::writeSfr(TdaSfr _addr, const std::initializer_list<uint8_t>& values)
{
  chDbgAssert(state == Tda5150State::READY, "not READY");
  uint8_t addr = static_cast<uint8_t>(_addr);
  
  chDbgAssert(addr >= 0x04 && (addr + values.size()) <= 0x27,
	      "incorrect sfr address range");

  addr &= 0b00111111;
  select();

  spi_lld_polled_send(&spid, addr);
  lchksum ^= addr;
  for (uint8_t b : values) {
    spi_lld_polled_send(&spid, b);
    lchksum ^= b;
  }  
  unselect();
}


uint8_t Tda5150::readSfr(TdaSfr _addr)
{
  uint8_t oneV{};
  chDbgAssert(state == Tda5150State::READY, "not READY");
  uint8_t addr = static_cast<uint8_t>(_addr);
  chDbgAssert(addr <= 0x27, "incorrect register address range");
  addr &= 0b00111111;
  addr |= 0b01000000;
  select();
  spi_lld_polled_send(&spid, addr);
  oneV = spi_lld_polled_receive(&spid);
  unselect();
  return oneV;
}

void Tda5150::writeSfr(TdaSfr addr, uint8_t value)
{
  writeSfr(addr, {value});
}

// one can use TransmitMask bitmask for mode
void Tda5150::startTransmit(uint8_t mode){
  chDbgAssert(state == Tda5150State::READY, "not READY");
  mode |= TRANSMIT_BITMASK;
  select();
  spi_lld_polled_send(&spid, mode);
  state = Tda5150State::SENDING;
  chThdSleepMicroseconds(100); // cf tda5150 ref manuel §2.4.3.4 Transmit Command
  tiedTxMosi.select(lineTx);
#if TIED_CLOCK
  tiedCkClk.select(lineCk);
#endif
}

void Tda5150::endTransmit(){
  chDbgAssert(state == Tda5150State::SENDING, "not SENDING");
  unselect();
  tiedTxMosi.select(lineMosi);
#if TIED_CLOCK
  tiedCkClk.select(lineClk);
#endif
  state = Tda5150State::READY;
}

bool  Tda5150::chksumValid()
{
  uint8_t tdaChksum = readSfr(TdaSfr::SPICHKSUM);
  const bool status = tdaChksum == lchksum;
#ifdef TRACE
  if (tdaChksum != lchksum) {
    DebugTrace("tdaChksum 0x%x != local 0x%x", tdaChksum, lchksum);
  }
#endif
  lchksum = 0;
  return status;
}
