#pragma once

#include <ch.h>
#include <hal.h>
#include <array>
#include "tiedGpios.hpp"


/*
  Il faut en paramètre du constructeur une struct spiconfig, une struct SPIDriver
  la line de enable et celle qui est partagée par miso, mosi et data

  ° private fn :
	ø initialiser les registres SFR
	ø ecrire un registre SFR
	ø lire un registre SFR
	ø ecrire un buffer de registres SFR contigus 
	ø lire un buffer de registres SFR contigus 
	ø ecrire une liste de couple adresse SFR, valeur
	ø check concordence entre cksum local et cksum distant

  ° 


 */

using tda_sfr_addr_t = uint8_t;

enum class Tda5150State {UNINIT, READY, SENDING};

class Tda5150 {
private:
  static constexpr SPIConfig spicfg {
    .circular = false,
    .slave = false,
    .data_cb = NULL,
    .error_cb = NULL,
  
    // CPOL=0, CPHA = 1 : SCK idle is low, read is done on SCK falling edge
    // SPI frequency 1.25Mhz :  < Max 2Mhz
    .cr1 = SPI_CR1_CPHA |
           SPI_CR1_BR_2 | SPI_CR1_BR_0 |
           SPI_CR1_BIDIMODE,
    .cr2 = 0
  };
  public:
  struct AddrVal {
    uint8_t addr;
    uint8_t val;
  };

  Tda5150(SPIDriver& _spid, ioline_t _enable, 
	  ioline_t _lineMosi, uint32_t afMosi,
	  ioline_t _lineTx, uint32_t afTx) :
    spid(_spid), 
    enable(_enable), lineMosi(_lineMosi), lineTx(_lineTx),
    tiedPins{
      {_lineMosi,
       PAL_MODE_ALTERNATE(afMosi) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST},
      
      {_lineTx,
       PAL_MODE_ALTERNATE(afTx) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST}
    },
    lcksum(0) {
    initSpi();
    initTda5150();
    state = Tda5150State::READY;
  }
  void startFrame(uint8_t mode);
  void endFrame();
  bool cksumValid();

  
  private:
  void initSpi();
  void initTda5150();

  void writeSfr(const std::initializer_list<AddrVal>& values);
  void writeSfr(tda_sfr_addr_t addr, const std::initializer_list<uint8_t>& values);

  void writeSfr(tda_sfr_addr_t addr, uint8_t value);
  uint8_t readSfr(tda_sfr_addr_t addr);
  void select() {palSetLine(enable);}
  void unselect() {palClearLine(enable);}
  void modeOut() {spid.spi->CR1 |= SPI_CR1_BIDIOE;}
  void modeIn() {spid.spi->CR1 &= ~SPI_CR1_BIDIOE;}
  
  
  SPIDriver& spid;
  ioline_t enable;
  ioline_t lineMosi;
  ioline_t lineTx;
  TiedPins<2> tiedPins;
  uint8_t  lcksum;
  Tda5150State state = Tda5150State::UNINIT;
};


