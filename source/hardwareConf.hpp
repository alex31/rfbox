#pragma once
#include "crcv1.h"



inline constexpr uint32_t fPclk = STM32_PCLK1;
    

#define CONCAT_NX(st1, st2) st1 ## st2
#define CONCAT(st1, st2) CONCAT_NX(st1, st2)

#define INVERT_OOK_MODUL true
#define PARANOID_REGREAD false
#define PACKET_EMISSION_READ_FROM_SERIAL true

inline constexpr size_t oledWidth = 22U;
inline constexpr size_t oledHeight = 4U;

constexpr uint32_t getBr12(uint32_t speed)
{
    uint32_t br12 = 0;
    while (fPclk / (1U << (br12+1U)) > speed) {
      if (++br12 == 7) break;
    }
    return br12 << SPI_CR1_BR_Pos;
}

inline constexpr CRCConfig crcCfgModbus = {
    .poly_size = 16,
    .poly = 0x8005,
    .initial_val = 0xffff,
    .final_val = 0x0000,
    .reflect_data = true,
    .reflect_remainder = true
  };

inline constexpr uint32_t spiBaudRate = 6'000'000;
inline constexpr uint32_t carrierFrequencyLow = 868'000'000;
inline constexpr uint32_t carrierFrequencyHigh = 870'000'000;
inline constexpr int8_t   ampLevelDbLow = 0;
inline constexpr int8_t   ampLevelDbHigh = 18;
inline constexpr uint32_t baudLow = 4800;
inline constexpr uint32_t baudHigh = 19200;
inline constexpr uint32_t fskBroadcastBitRate = (baudHigh * 3) / 2;
inline constexpr SerialDriver &SD_METEO = CONCAT(SD, EXTVCP_TX_USART);
inline constexpr uint32_t warmBootWdg = 0xDEADC0DE;
inline constexpr uint32_t warmBootSysRst = 0xBADCAFFE;

// Calculated constant

inline constexpr uint32_t spiBr12Baud = getBr12(spiBaudRate);


