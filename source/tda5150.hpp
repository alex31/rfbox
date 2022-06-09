#pragma once

#include <ch.h>
#include <hal.h>
#include <array>
#include "tiedGpios.hpp"
#include "common.hpp"

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
	ø check concordence entre chksum local et chksum distant

  ° 


 */


enum TxstatMask : uint8_t {TXSTAT_PLLLDER = 1 << 0, TXSTAT_PARERR = 1 << 1,
  TXSTAT_BROUTERR = 1 << 2, TXSTAT_LBD_2V4 = 1 << 4, TXSTAT_LBD_2V1 = 1 << 5,
  TXSTAT_MASK = 0b1110110}; 

enum TransmitMask : uint8_t {
  TRANSMIT_CHAN_A = 0b00,
  TRANSMIT_CHAN_B = 0b01,
  TRANSMIT_CHAN_C = 0b10,
  TRANSMIT_CHAN_D = 0b11,
  TRANSMIT_POWER_LEVEL_1 = 0,
  TRANSMIT_POWER_LEVEL_2 = 0b100,
  TRANSMIT_ENCODING_OFF = 0,
  TRANSMIT_ENCODING_ON = 0b1000,
  TRANSMIT_PAMODE_0 = 0,
  TRANSMIT_PAMODE_1 = 0b10000,
  TRANSMIT_DATASYNC_OFF = 0,
  TRANSMIT_DATASYNC_ON = 0b100000,
  TRANSMIT_BITMASK = 0b11000000
};

enum class TdaSfr : uint8_t {
  SPICHKSUM = 0x0,
  TXSTAT = 0x1,
  TXCFG0 = 0x4,
  TXCFG1,
  CLKOUTCFG,
  BDRDIV,
  PRBS,
  PLLINTA,
  PLLFRACA0,
  PLLFRACA1,
  PLLFRACA2,
  PLLINTB,
  PLLFRACB0,
  PLLFRACB1,
  PLLFRACB2,
  PLLINTC,
  PLLFRACC0,
  PLLFRACC1,
  PLLFRACC2,
  PLLINTD,
  PLLFRACD0,
  PLLFRACD1,
  PLLFRACD2,
  SLOPEDIV,
  POWCFG0,
  POWCFG1,
  FDEV,
  GFDIV,
  GFXOSC,
  ANTTDCC,
  RES1,
  VAC0,
  VAC1,
  VACERRTH,
  CPCFG,
  PLLBW,
  RES2,
  ENCCNT
};

static_assert(static_cast<int>(TdaSfr::ENCCNT) == 0x27);
enum class Tda5150State {UNINIT, READY, SENDING};

class Tda5150 {
private:
 static constexpr SPIConfig spicfg {
    .circular = false,
    .slave = false,
    .data_cb = NULL,
    .error_cb = [](SPIDriver *) {chSysHalt("spi hard fault");},
      
      // CPOL=0, CPHA = 1 : SCK idle is low, read is done on SCK falling edge
      // SPI frequency 1.25Mhz :  < Max 2Mhz
    .cr1 = SPI_CR1_CPHA | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE |
           SPI_CR1_BR_2 | SPI_CR1_BR_0,
    .cr2 = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
 };
public:
  struct AddrVal {
    TdaSfr  addr;
    uint8_t val;
  };

#if TIED_CLOCK
  Tda5150(SPIDriver& _spid, ioline_t _enable, 
	  ioline_t _lineMosi, uint32_t afMosi,
	  ioline_t _lineTx, uint32_t afTx,
          ioline_t _lineClk, uint32_t afClk,
	  ioline_t _lineCk, uint32_t afCk) :
    spid(_spid), 
    enable(_enable), lineMosi(_lineMosi), lineTx(_lineTx),
    lineClk(_lineClk), lineCk(_lineCk),
    tiedTxMosi{
      {_lineMosi,
       PAL_MODE_ALTERNATE(afMosi) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST},
      
      {_lineTx,
       PAL_MODE_ALTERNATE(afTx) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST}
    },
   tiedCkClk{
      {_lineClk,
       PAL_MODE_ALTERNATE(afClk) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST},
      
      {_lineCk,
       PAL_MODE_ALTERNATE(afCk) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST}
    },
#else
  Tda5150(SPIDriver& _spid, ioline_t _enable, 
	  ioline_t _lineMosi, uint32_t afMosi,
	  ioline_t _lineTx, uint32_t afTx) :
    spid(_spid), 
    enable(_enable), lineMosi(_lineMosi), lineTx(_lineTx),
    tiedTxMosi{
      {_lineMosi,
       PAL_MODE_ALTERNATE(afMosi) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST},
      
      {_lineTx,
       PAL_MODE_ALTERNATE(afTx) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST}
    },
#endif
    lchksum(0) {
    state = Tda5150State::READY;
  }
  void init(void) {initSpi();}
  void startTransmit(uint8_t mode);
  void endTransmit();
  bool chksumValid();
  TxstatMask getTxStatus(void) {return static_cast<TxstatMask>(readSfr(TdaSfr::TXSTAT));}
  void writeSfr(const std::initializer_list<AddrVal>& values);
  void writeSfr(TdaSfr addr, const std::initializer_list<uint8_t>& values);

  void writeSfr(TdaSfr addr, uint8_t value);
  uint8_t readSfr(TdaSfr addr);
  
  private:
  void initSpi();

  void select() {palSetLine(enable);}
  void unselect() {palClearLine(enable);}
  
  
  SPIDriver& spid;
  ioline_t enable;
  ioline_t lineMosi;
  ioline_t lineTx;
#if TIED_CLOCK
  ioline_t lineClk;
  ioline_t lineCk;
#endif
  TiedPins<2> tiedTxMosi;
#if TIED_CLOCK
  TiedPins<2> tiedCkClk;
#endif
  uint8_t  lchksum;
  Tda5150State state = Tda5150State::UNINIT;
};


