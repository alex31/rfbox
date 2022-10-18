#pragma once
#include "crcv1.h"
#include <array>


#define CONCAT_NX(st1, st2) st1 ## st2
#define CONCAT(st1, st2) CONCAT_NX(st1, st2)

constexpr bool invertOokModulation = true;
constexpr bool paranoidRegisterRead = false;

inline constexpr size_t oledWidth = 22U;
inline constexpr size_t oledHeight = 4U;

enum class BitRateIndex {Low, High, VeryHigh, UpBound};

constexpr std::underlying_type_t<BitRateIndex> operator+(BitRateIndex i) noexcept
{
    return static_cast<std::underlying_type_t<BitRateIndex>>(i);
}

constexpr BitRateIndex operator++(BitRateIndex i) noexcept
{
  const std::underlying_type_t<BitRateIndex> utv =
    static_cast<std::underlying_type_t<BitRateIndex>>(i) + 1U; 
  return static_cast<BitRateIndex>(utv);
}

/*
  find the bitmask for SPI clock divider. the corresponding cleock will be the higher value
  that is les or equal ask frequency
 */
constexpr uint32_t getBr12(uint32_t speed)
{
  constexpr uint32_t fPclk = STM32_PCLK1;
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

// editable constant
inline constexpr uint32_t spiBaudRate = 6'000'000;
inline constexpr uint32_t carrierFrequencyLow = 868'000'000;
inline constexpr uint32_t carrierFrequencyHigh = 870'000'000;
inline constexpr int8_t   ampLevelDbLow = 0;
inline constexpr int8_t   ampLevelDbHigh = 18;
inline constexpr std::array<uint32_t, +BitRateIndex::UpBound> baudRates = {
  4800, 19200, /*57600*/ 115200};
inline constexpr float    fskBroadcastLowBitRate_ratio = 1.3f;
inline constexpr float    fskBroadcastHighBitRate_ratio = 1.1f;

// Calculated constant
inline constexpr uint32_t spiBr12Baud = getBr12(spiBaudRate);

// non editable constants
inline constexpr SerialDriver &SD_METEO = CONCAT(SD, EXTVCP_TX_USART);
inline constexpr uint32_t warmBootWdg = 0xDEADC0DE;
inline constexpr uint32_t warmBootSysRst = 0xBADCAFFE;

