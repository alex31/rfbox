#include "tda5150.hpp"
#include "stdutil.h"
#include <array>

void Tda5150::initSpi()
{
  unselect();
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
  std::array<uint8_t, 36> spiBuffer{};
  chDbgAssert(state == Tda5150State::READY, "not READY");
  uint8_t addr = static_cast<uint8_t>(_addr);
  
  chDbgAssert(addr >= 0x04 && (addr + values.size()) <= 0x27,
	      "incorrect sfr address range");

  addr &= 0b00111111;
  modeOut();

  spiBuffer[0] = addr;
  std::copy(values.begin(), values.end(), spiBuffer.begin() + 1);
  for (size_t i=0; i<spiBuffer.size(); i++) {
    DebugTrace("value[%u] = 0x%x", i, spiBuffer[i]);
  }
  spiSend(&spid, values.size() + 1, spiBuffer.data());
  lcksum ^= addr;
  for (const auto v : values)
    lcksum ^= v;
  
  modeIdle();
}


uint8_t Tda5150::readSfr(TdaSfr _addr)
{
  uint8_t oneV{};
  chDbgAssert(state == Tda5150State::READY, "not READY");
  uint8_t addr = static_cast<uint8_t>(_addr);
  chDbgAssert(addr <= 0x27, "incorrect register address range");
  addr &= 0b00111111;
  addr |= 0b01000000;
  modeOut();
  spiSend(&spid, sizeof(addr), &addr);
  modeIdle();
  modeIn();
  spiReceive(&spid, sizeof(oneV), &oneV);
  modeIdle();
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
  modeIdle();
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
