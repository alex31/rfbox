#pragma once
#include <cstdint>
#define TIED_CLOCK false

static constexpr uint32_t UART_BAUD =  2400U;
static constexpr size_t   DATAFRAME_LEN =  14U;
static constexpr uint32_t DATAFRAME_FREQUENCY =  2U; // hertz

union dataFrame {
  struct {
    const uint16_t startBeacon = 0xcafe;
    int16_t temp; // degree * 10
    uint16_t pressure; // hectopascal * 10 
    uint16_t windSpeed; // centimeters/s
    uint16_t windDir; // degrées 0 - 360
    uint16_t flags; // bit 0 is one if mast is NOT vertical
    uint16_t crc; // crc16
  };
  uint8_t raw[14];
};
