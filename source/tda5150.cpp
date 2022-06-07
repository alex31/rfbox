#include "tda5150.hpp"
#include "stdutil.h"

void Tda5150::initSpi()
{
  spiStart(&spid, &spicfg);
  unselect();
  tiedPins.select(lineMosi);
}

void Tda5150::initTda5150()
{
  // initialize all SFR registers

  writeSfr(0x04, {0x10, 0x11});
  writeSfr(0x04, 0x10);
  DebugTrace("status = %u",   readSfr(0x04));
}

void Tda5150::writeSfr(const std::initializer_list<AddrVal>& values)
{
  for (const auto [a, v] : values) {
    writeSfr(a, v);
  }

}

void Tda5150::writeSfr(tda_sfr_addr_t addr, const std::initializer_list<uint8_t>& values)
{
  chDbgAssert(state == Tda5150State::READY, "not READY");
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


uint8_t Tda5150::readSfr(tda_sfr_addr_t addr)
{
  uint8_t oneV{};
  chDbgAssert(state == Tda5150State::READY, "not READY");
  chDbgAssert(addr >= 0x04 && addr <= 0x27,
	      "incorrect sfr address range");
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

void Tda5150::writeSfr(tda_sfr_addr_t addr, uint8_t value)
{
  writeSfr(addr, {value});
}


void Tda5150::startFrame(uint8_t mode){
  chDbgAssert(state == Tda5150State::READY, "not READY");
  mode |= 0b11000000;
  select();
  modeOut();
  spiSend(&spid, sizeof(mode), &mode);
  state = Tda5150State::SENDING;
  chThdSleepMicroseconds(100); // cf tda5150 ref manuel §2.4.3.4 Transmit Command
  tiedPins.select(lineTx);
}

void Tda5150::endFrame(){
  chDbgAssert(state == Tda5150State::SENDING, "not SENDING");
  unselect();
  tiedPins.select(lineMosi);
}

bool  Tda5150::cksumValid()
{
  uint8_t tdaCksum = readSfr(0x00);
  const bool status = tdaCksum == lcksum;
  lcksum = 0;
  return status;
}
