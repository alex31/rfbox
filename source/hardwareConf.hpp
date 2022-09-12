#pragma once

#ifdef STM32F4xx_MCUCONF
#define LINE_MCU_RX    LINE_EXTVCP_RX
#define AF_LINE_MCU_RX AF_LINE_EXTVCP_RX
inline constexpr uint32_t fPclk = STM32_PCLK2;
#else
#define LINE_MCU_RX    LINE_EXTVCP_TX
#define AF_LINE_MCU_RX AF_LINE_EXTVCP_TX
inline constexpr uint32_t fPclk = STM32_PCLK1;
#endif
    

#define CONCAT_NX(st1, st2) st1 ## st2
#define CONCAT(st1, st2) CONCAT_NX(st1, st2)

#define DIO2_DIRECT true
#define INVERT_UART_LEVEL true
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

inline constexpr uint32_t spiBaudRate = 1'500'000;
inline constexpr uint32_t carrierFrequencyLow = 868'000'000;
inline constexpr uint32_t carrierFrequencyHigh = 870'000'000;
inline constexpr int8_t   ampLevelDbLow = 0;
inline constexpr int8_t   ampLevelDbHigh = 18;
inline constexpr uint32_t baudLow = 4800;
inline constexpr uint32_t baudHigh = 19200;
inline constexpr SerialDriver &SD_METEO = CONCAT(SD, EXTVCP_TX_USART);


// Calculated constant

inline constexpr uint32_t spiBr12Baud = getBr12(spiBaudRate);


