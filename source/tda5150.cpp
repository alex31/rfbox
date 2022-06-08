#include "tda5150.hpp"
#include "stdutil.h"

void Tda5150::initSpi()
{
  spiStart(&spid, &spicfg);
  unselect();
  tiedTxMosi.select(lineMosi);
#if TIED_CLOCK
  tiedCkClk.select(lineClk);
#endif
}

void Tda5150::initTda5150()
{
  // initialize all SFR registers

  writeSfr(TdaSfr::TXCFG1, {0x10, 0x11});
  writeSfr(TdaSfr::PLLINTD, 0x10);
  writeSfr({{TdaSfr::PLLFRACB0, 0x1}, {TdaSfr::PLLFRACB1, 0x2}});
  DebugTrace("status = %u",   readSfr(TdaSfr::TXCFG1));
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
  modeOut();
  spiSend(&spid, sizeof(addr), &addr);
  spiSend(&spid, values.size(), std::data(values));
  lcksum ^= addr;
  for (const auto v : values)
    lcksum ^= v;
  
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
  modeOut();
  spiSend(&spid, sizeof(addr), &addr);
  modeIn();
  spiReceive(&spid, sizeof(oneV), &oneV);
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
  modeOut();
  spiSend(&spid, sizeof(mode), &mode);
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

bool  Tda5150::cksumValid()
{
  uint8_t tdaCksum = readSfr(TdaSfr::SPICHKSUM);
  const bool status = tdaCksum == lcksum;
  lcksum = 0;
  return status;
}
