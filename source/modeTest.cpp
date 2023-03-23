#include "modeTest.hpp"
#include "bboard.hpp"
#include "bitIntegrator.hpp"
#include "ch.h"
#include "etl/string.h"
#include "frozen/map.h"
#include "hal.h"
#include "hardwareConf.hpp"
#include "radio.hpp"
#include "stdutil.h"
#include <array>
#include <bit>
#include <limits>

namespace {
using ErrorString = etl::string<48>;
THD_WORKING_AREA(waAutonomousTest, 1280);

SerialConfig meteoSerialConfig = {
    .speed = baudRates[+BitRateIndex::Low],
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN | invertOokModulation
               ? (USART_CR2_TXINV | USART_CR2_RXINV)
               : 0,
    .cr3 = 0};

bool started = false;
void autonomousTestWrite(void *);
void autonomousTestRead(void *);
Integrator<1024> integ;
constexpr uint8_t frameLength = 160U;

systime_t timoutTs = 0;

constexpr std::array<uint16_t, 195> seq2dcfGen = {
    0x25ad, 0x25b5, 0x25b6, 0x26ad, 0x26b5, 0x26b6, 0x29ad, 0x29b5, 0x29b6,
    0x2aad, 0x2ab5, 0x2ab6, 0x2b95, 0x2b96, 0x2b99, 0x2b9a, 0x2ba5, 0x2ba6,
    0x2ba9, 0x2baa, 0x2bb2, 0x2cad, 0x2cb5, 0x2cb6, 0x2d95, 0x2d96, 0x2d99,
    0x2d9a, 0x2da5, 0x2da6, 0x2da9, 0x2daa, 0x2db2, 0x32ad, 0x32b5, 0x32b6,
    0x3395, 0x3396, 0x3399, 0x339a, 0x33a5, 0x33a6, 0x33a9, 0x33aa, 0x33b2,
    0x34ad, 0x34b5, 0x34b6, 0x3595, 0x3596, 0x3599, 0x359a, 0x35a5, 0x35a6,
    0x35a9, 0x35aa, 0x35b2, 0x3695, 0x3696, 0x3699, 0x369a, 0x36a5, 0x36a6,
    0x36a9, 0x36aa, 0x36b2, 0x49ad, 0x49b5, 0x49b6, 0x4aad, 0x4ab5, 0x4ab6,
    0x4b95, 0x4b96, 0x4b99, 0x4b9a, 0x4ba5, 0x4ba6, 0x4ba9, 0x4baa, 0x4bb2,
    0x4cad, 0x4cb5, 0x4cb6, 0x4d95, 0x4d96, 0x4d99, 0x4d9a, 0x4da5, 0x4da6,
    0x4da9, 0x4daa, 0x4db2, 0x52ad, 0x52b5, 0x52b6, 0x5395, 0x5396, 0x5399,
    0x539a, 0x53a5, 0x53a6, 0x53a9, 0x53aa, 0x53b2, 0x54ad, 0x54b5, 0x54b6,
    0x5595, 0x5596, 0x5599, 0x559a, 0x55a5, 0x55a6, 0x55a9, 0x55aa, 0x55b2,
    0x5695, 0x5696, 0x5699, 0x569a, 0x56a5, 0x56a6, 0x56a9, 0x56aa, 0x56b2,
    0x5995, 0x5996, 0x5999, 0x599a, 0x59a5, 0x59a6, 0x59a9, 0x59aa, 0x59b2,
    0x5a95, 0x5a96, 0x5a99, 0x5a9a, 0x5aa5, 0x5aa6, 0x5aa9, 0x5aaa, 0x5ab2,
    0x5b92, 0x64ad, 0x64b5, 0x64b6, 0x6595, 0x6596, 0x6599, 0x659a, 0x65a5,
    0x65a6, 0x65a9, 0x65aa, 0x65b2, 0x6695, 0x6696, 0x6699, 0x669a, 0x66a5,
    0x66a6, 0x66a9, 0x66aa, 0x66b2, 0x6995, 0x6996, 0x6999, 0x699a, 0x69a5,
    0x69a6, 0x69a9, 0x69aa, 0x69b2, 0x6a95, 0x6a96, 0x6a99, 0x6a9a, 0x6aa5,
    0x6aa6, 0x6aa9, 0x6aaa, 0x6ab2, 0x6b92, 0x6c95, 0x6c96, 0x6c99, 0x6c9a,
    0x6ca5, 0x6ca6, 0x6ca9, 0x6caa, 0x6cb2, 0x6d92};
  
constexpr frozen::map<uint16_t, uint8_t, 195> dcf2seqGen = {
    {0x25ad, 0},   {0x25b5, 1},   {0x25b6, 2},   {0x26ad, 3},   {0x26b5, 4},
    {0x26b6, 5},   {0x29ad, 6},   {0x29b5, 7},   {0x29b6, 8},   {0x2aad, 9},
    {0x2ab5, 10},  {0x2ab6, 11},  {0x2b95, 12},  {0x2b96, 13},  {0x2b99, 14},
    {0x2b9a, 15},  {0x2ba5, 16},  {0x2ba6, 17},  {0x2ba9, 18},  {0x2baa, 19},
    {0x2bb2, 20},  {0x2cad, 21},  {0x2cb5, 22},  {0x2cb6, 23},  {0x2d95, 24},
    {0x2d96, 25},  {0x2d99, 26},  {0x2d9a, 27},  {0x2da5, 28},  {0x2da6, 29},
    {0x2da9, 30},  {0x2daa, 31},  {0x2db2, 32},  {0x32ad, 33},  {0x32b5, 34},
    {0x32b6, 35},  {0x3395, 36},  {0x3396, 37},  {0x3399, 38},  {0x339a, 39},
    {0x33a5, 40},  {0x33a6, 41},  {0x33a9, 42},  {0x33aa, 43},  {0x33b2, 44},
    {0x34ad, 45},  {0x34b5, 46},  {0x34b6, 47},  {0x3595, 48},  {0x3596, 49},
    {0x3599, 50},  {0x359a, 51},  {0x35a5, 52},  {0x35a6, 53},  {0x35a9, 54},
    {0x35aa, 55},  {0x35b2, 56},  {0x3695, 57},  {0x3696, 58},  {0x3699, 59},
    {0x369a, 60},  {0x36a5, 61},  {0x36a6, 62},  {0x36a9, 63},  {0x36aa, 64},
    {0x36b2, 65},  {0x49ad, 66},  {0x49b5, 67},  {0x49b6, 68},  {0x4aad, 69},
    {0x4ab5, 70},  {0x4ab6, 71},  {0x4b95, 72},  {0x4b96, 73},  {0x4b99, 74},
    {0x4b9a, 75},  {0x4ba5, 76},  {0x4ba6, 77},  {0x4ba9, 78},  {0x4baa, 79},
    {0x4bb2, 80},  {0x4cad, 81},  {0x4cb5, 82},  {0x4cb6, 83},  {0x4d95, 84},
    {0x4d96, 85},  {0x4d99, 86},  {0x4d9a, 87},  {0x4da5, 88},  {0x4da6, 89},
    {0x4da9, 90},  {0x4daa, 91},  {0x4db2, 92},  {0x52ad, 93},  {0x52b5, 94},
    {0x52b6, 95},  {0x5395, 96},  {0x5396, 97},  {0x5399, 98},  {0x539a, 99},
    {0x53a5, 100}, {0x53a6, 101}, {0x53a9, 102}, {0x53aa, 103}, {0x53b2, 104},
    {0x54ad, 105}, {0x54b5, 106}, {0x54b6, 107}, {0x5595, 108}, {0x5596, 109},
    {0x5599, 110}, {0x559a, 111}, {0x55a5, 112}, {0x55a6, 113}, {0x55a9, 114},
    {0x55aa, 115}, {0x55b2, 116}, {0x5695, 117}, {0x5696, 118}, {0x5699, 119},
    {0x569a, 120}, {0x56a5, 121}, {0x56a6, 122}, {0x56a9, 123}, {0x56aa, 124},
    {0x56b2, 125}, {0x5995, 126}, {0x5996, 127}, {0x5999, 128}, {0x599a, 129},
    {0x59a5, 130}, {0x59a6, 131}, {0x59a9, 132}, {0x59aa, 133}, {0x59b2, 134},
    {0x5a95, 135}, {0x5a96, 136}, {0x5a99, 137}, {0x5a9a, 138}, {0x5aa5, 139},
    {0x5aa6, 140}, {0x5aa9, 141}, {0x5aaa, 142}, {0x5ab2, 143}, {0x5b92, 144},
    {0x64ad, 145}, {0x64b5, 146}, {0x64b6, 147}, {0x6595, 148}, {0x6596, 149},
    {0x6599, 150}, {0x659a, 151}, {0x65a5, 152}, {0x65a6, 153}, {0x65a9, 154},
    {0x65aa, 155}, {0x65b2, 156}, {0x6695, 157}, {0x6696, 158}, {0x6699, 159},
    {0x669a, 160}, {0x66a5, 161}, {0x66a6, 162}, {0x66a9, 163}, {0x66aa, 164},
    {0x66b2, 165}, {0x6995, 166}, {0x6996, 167}, {0x6999, 168}, {0x699a, 169},
    {0x69a5, 170}, {0x69a6, 171}, {0x69a9, 172}, {0x69aa, 173}, {0x69b2, 174},
    {0x6a95, 175}, {0x6a96, 176}, {0x6a99, 177}, {0x6a9a, 178}, {0x6aa5, 179},
    {0x6aa6, 180}, {0x6aa9, 181}, {0x6aaa, 182}, {0x6ab2, 183}, {0x6b92, 184},
    {0x6c95, 185}, {0x6c96, 186}, {0x6c99, 187}, {0x6c9a, 188}, {0x6ca5, 189},
    {0x6ca6, 190}, {0x6ca9, 191}, {0x6caa, 192}, {0x6cb2, 193}, {0x6d92, 194}};

} // namespace

namespace ModeTest {
float getBer(void) {
  const float ber = integ.getAvg() * 1000.0f;
  board.setBer(ber);
  return ber;
}

void start(RfMode rfMode, uint32_t baud) {
  board.setDio2Threshold({0.48f, 0.62f});
  meteoSerialConfig.speed = baud;
  // DIO is connected on UART1_TX
  if (rfMode == RfMode::RX)
    meteoSerialConfig.cr2 |= USART_CR2_SWAP;

  sdStart(&SD_METEO, &meteoSerialConfig);
  if (not started) {
    started = true;
    if (rfMode == RfMode::TX) {
      chThdCreateStatic(waAutonomousTest, sizeof(waAutonomousTest), NORMALPRIO,
                        &autonomousTestWrite, nullptr);
    } else if (rfMode == RfMode::RX) {
      chThdCreateStatic(waAutonomousTest, sizeof(waAutonomousTest), NORMALPRIO,
                        &autonomousTestRead, nullptr);
    } else {
      chSysHalt("invalid rfMode");
    }
  }
}
} // namespace ModeTest

namespace {

void autonomousTestWrite(void *) {
  chRegSetThreadName("autonomousTestWrite");
  while (true) {
    sdWrite(&SD_METEO, reinterpret_cast<const uint8_t *>(seq2dcfGen.data()),
            seq2dcfGen.size() * sizeof(seq2dcfGen[0]));
  }
}

void autonomousTestRead(void *) {
  chRegSetThreadName("autonomousTestRead");
  uint8_t expectedByte = 0;
  uint32_t zeroInRow = 0;
  systime_t ts = chVTGetSystemTimeX();

  while (true) {
    const int lsb = sdGetTimeout(&SD_METEO, TIME_MS2I(200));
    if (const systime_t now = chVTGetSystemTimeX();
        chTimeDiffX(ts, now) > TIME_MS2I(500)) {
      ts = now;
      ModeTest::getBer();
    }
    if (lsb < 0) {
      if ((++zeroInRow) > 100U) {
        zeroInRow = 0;
        DebugTrace("problem detected : Read Timeout");
        board.setError("Read Timeout");
      }

      if (chTimeDiffX(timoutTs, chVTGetSystemTimeX()) > TIME_S2I(5)) {
        timoutTs = 0;
      } else {
        timoutTs = chVTGetSystemTimeX();
      }
      board.setError("RX timeout");
      integ.push(true);
      continue;
    } else if (lsb == 0) {
      if ((++zeroInRow) > 10U) {
        integ.push(true);
        zeroInRow = 0;
        DebugTrace("problem detected : Read only 0");
        board.setError("Read only 0");
        //	  Radio::radio.calibrate();
      }
    } else {
      zeroInRow = 0;
      board.clearError();
      timoutTs = 0;
      if (lsb < 0x80)
        continue;
      const int msb = sdGetTimeout(&SD_METEO, TIME_MS2I(200));
      if (msb <= 0)
        continue;
      uint16_t balancedWord = (lsb & 0xff) | ((msb & 0xff) << 8);
      uint8_t c = 0;
      if (dcf2seqGen.contains(balancedWord)) {
        c = dcf2seqGen.at(balancedWord);
      } else {
        integ.push(true);
        continue;
      }
      integ.push(c != expectedByte);
      expectedByte = (c + 1) % dcf2seqGen.size();
    }
  }
}

} // namespace
