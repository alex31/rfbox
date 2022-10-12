#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

enum class Rfm69RegIndex : uint8_t {
  Fifo,
  RfMode, First = RfMode,
  DataModul,
  Bitrate,
  Fdev,
  Frf = 0x7,
  Osc1 = 0x0A,
  AfcCtrl,
  Listen1 = 0xD,
  Listen2,
  Listen3,
  Version,
  PaLevel,
  PaRamp,
  Ocp,
  Lna = 0x18,
  RxBw,
  AfcBw,
  OokPeak,
  OokAvg,
  OokFix,
  AfcFei,
  AfcMsb,
  AfcLsb,
  FeiMsb,
  FeiLsb,
  RssiConfig,
  RssiValue,
  DioMapping,
  IrqFlags1 = 0x27,
  IrqFlags2,
  RssiThresh,
  RxTimeout1,
  RxTimeout2,
  PreambleSize,
  SyncConfig = 0x2E,
  SyncValue,
  PacketConfig1 = 0x37,
  PayloadLength,
  NodeAdrs,
  BroadcastAdrs,
  AutoModes,
  FifoThresh,
  PacketConfig2,
  AesKey,
  Temp1 = 0x4E,
  Temp2,
  TestLna = 0x58,
  TestPa1 = 0x5A,
  TestPa2 = 0x5C,
  TestDagc = 0x6F,
  TestAfc = 0x71,
  Last = TestAfc
};

constexpr auto operator-(Rfm69RegIndex a, Rfm69RegIndex b) noexcept
{
    return static_cast<std::underlying_type_t<Rfm69RegIndex>>(a) -
      static_cast<std::underlying_type_t<Rfm69RegIndex>>(b);
}


static_assert(static_cast<uint8_t>(Rfm69RegIndex::AesKey) == 0x3E);
static_assert(static_cast<uint8_t>(Rfm69RegIndex::DioMapping) == 0x25);

enum class RfMode : uint8_t {SLEEP, STDBY, FS, TX, RX};
enum class DataModul : uint8_t {
  FSK_NOSHAPING=0b00000, FSK_BT_1P0=0b01000,
  FSK_BT_0P5=0b10000,FSK_BT_0P3=0b11000,
  OOK_NOSHAPING=0b00001, OOK_BR=0b01001, OOK_2BR=0b10001
};
enum class DataMode : uint8_t {
  PACKET=0b00, CONTINUOUS_SYNC=0b10, CONTINUOUS_NOSYNC=0b11};
  
enum class ListenEnd : uint8_t {
  STOP, GOTOMODE, RESUME_IDLE};

enum class ListenResolution : uint8_t {
  US_64=0b01, MS_4P1, MS_262};

enum class RampTime : uint8_t {
  MS_3P4, MS_2P0, MS_1P0, US_500, US_250, US_125, US_100, US_62,
  US_50, US_40, US_31, US_25, US_20, US_15, US_12, US_10};

enum class LnaGain : uint8_t {
  AGC, HIGHEST, MINUS_6, MINUS_12, MINUS_24, MINUS_36, MINUS_48};

enum class LnaInputImpedance : uint8_t {
  OHMS_50, OHMS_200};

enum class BandwithMantissa : uint8_t {
  MANT_16, MANT_20, MANT_24};

enum class ThresholdDec  : uint8_t {
  ONE, HALF, QUARTER, EIGHTH, TWICE, FOR_TIMES,
  EIGHT_TIMES, SIXTEEN_TIMES
};

enum class ThresholdStep  : uint8_t {
  DB_0P5, DB_1, DB_1P5, DB_2, DB_3, DB_4, DB_5, DB_6
};

enum class ThresholdType  : uint8_t {
  FIXED, PEAK, AVERAGE
};

enum class ThresholdFilt  : uint8_t {
  DIV32, DIV8, DIV4, DIV2
};

enum class ClockOut  : uint8_t {
  OSC, OSCDIV2, OSCDIV4, OSCDIV8, OSCDIV16,
  OSCDIV32, RC, OFF 
};

enum class AddressFiltering  : uint8_t {
  NONE, NODE, NODE_AND_BROADCAST
};

enum class DCFree  : uint8_t {
  NONE, MANCHESTER, WHITENING
};

enum class PacketFormat  : uint8_t {
  FIXED_LEN, VARIABLE_LEN
};

enum class IntermediateMode  : uint8_t {
  SLEEP, STDBY, RX, TX
};

enum class ExitCondition  : uint8_t {
  NONE, FALL_FIFO_NOTEMPTY, RISE_FIFO_LEVEL, RISE_CRC_OK,
  RISE_PAYLOAD_READY, RISE_SYNC_ADDRESS, RISE_PACKET_SENT,
  RISE_TIMEOUT
};

enum class EnterCondition  : uint8_t {
  NONE, RISE_FIFO_NOTEMPTY, RISE_FIFO_LEVEL, RISE_CRC_OK,
  RISE_PAYLOAD_READY, RISE_SYNC_ADDRESS, RISE_PACKET_SENT,
  FALL_FIFO_NOTEMPTY
};

enum class TxStartCondition  : uint8_t {
  FIFO_LEVEL, FIFO_NOT_EMPTY
};

enum class FifoFillCondition  : uint8_t {
  SYNC_MATCHES, ALWAYS
};

enum class TestLna  : uint8_t {
  NORMAL = 0x1B, HIGH_SENSITIVITY = 0x2D
};

enum class TestPa1  : uint8_t {
  NORMAL_AND_RX = 0x55, BOOST_20DB = 0x5D
};

enum class TestPa2  : uint8_t {
  NORMAL_AND_RX = 0x70, BOOST_20DB = 0x7C
};

enum class FadingMargin  : uint8_t {
  NORMAL = 0x0, IMPROVE_LOW_BETA_ON = 0x20, IMPROVE_LOW_BETA_OFF = 0x30
};


struct Rfm69Rmap {
  union {
    struct {
      uint8_t fifo;   // not really a register : special case
      
    uint8_t:2;     // bits 0-1 unused
      RfMode opMode_mode:3;
      bool opMode_listenAbort:1;
      bool opMode_listenOn:1;
      bool opMode_sequencerOff:1;
      
      DataModul datamodul_shaping:5;
      DataMode datamodul_dataMode:2;
    uint8_t:1; // unused

      uint16_t bitrate; // big endian, (chip rate if manchester)

      // the 2 most significant bits are unused (zeroed) but
      // declaring the bitfield as fdev:14 mess the swap endianness
      // anyway, when keeping fdev in the limits, theses two bits are always 0
      uint16_t fdev:16; // big endian
      
      uint32_t frf:24; // big endian frequency carrier
      
    uint8_t:6; // unused
      bool osc1_calibDone:1;
      bool osc1_calibStart:1;

    uint8_t:5; // unused
      bool afcCtrl_lowBetaOn:1;
    uint8_t:2; // unused

      
    uint8_t:8; // reserved

    bool:1; // unused
      ListenEnd listen1_end:2;
      bool listen1_criteriaSyncMatched:1;
      ListenResolution listen1_resolutionRx:2;
      ListenResolution listen1_resolutionIdle:2;

      uint8_t listen2_coefIdle;
      uint8_t listen3_coefRx;
      uint8_t version;
      
      uint8_t paLevel_outputPower:5;
      bool paLevel_pa2On:1;
      bool paLevel_pa1On:1;
      bool paLevel_pa0On:1;
      
      RampTime paRamp:4;
      uint8_t:4; // unused
      
      uint8_t ocp_trim:4;
      bool ocp_on:1;
      uint8_t:3; // unused
      
    uint32_t:32; // reserved
      
      LnaGain lna_gain:3;
      LnaGain lna_currentGain:3;
    uint8_t:1; // unused
      LnaInputImpedance lna_zIn:1;

      uint8_t rxBw_exp:3;
      BandwithMantissa rxBw_mant:2;
      uint8_t rxBw_dccFreq:3;

      uint8_t afcBw_exp:3;
      BandwithMantissa afcBw_mant:2;
      uint8_t afcBw_dccFreq:3;

      ThresholdDec   ookPeak_threshDec:3;
      ThresholdStep  ookPeak_threshStep:3;
      ThresholdType  ookPeak_type:2;

      
    uint8_t:6; // unused
      ThresholdFilt ookAvg_threshFilt:2;

      uint8_t ookFix_threshold;
      
      bool afc_start:1;
      bool afc_clear:1;
      bool afc_autoOn:1;
      bool afc_autoClearOn:1;
      bool afc_done:1;
      bool afc_feiStart:1;
      bool afc_feiDone:1;
    uint8_t:1; // unused

      int16_t afc; // big endian
      int16_t fei; // big endian

      bool rssiConfig_start:1;
      bool rssiConfig_done:1;
    uint8_t:6; // unused
      
      uint8_t rssi;
      
      uint8_t dioMapping_io3:2;
      uint8_t dioMapping_io2:2;
      uint8_t dioMapping_io1:2;
      uint8_t dioMapping_io0:2;

      
      ClockOut dioMapping_clkOut:3;
    uint8_t:1; // unused
      uint8_t dioMapping_io5:2;
      uint8_t dioMapping_io4:2;

      union {
	struct {
	  bool irqFlags_syncAddressMatch:1;
	  bool irqFlags_autoMode:1;
	  bool irqFlags_timeOut:1;
	  bool irqFlags_rssi:1;
	  bool irqFlags_pllLock:1;
	  bool irqFlags_txReady:1;
	  bool irqFlags_rxReady:1;
	  bool irqFlags_modeReady:1;
	};
	uint8_t irqFlags1;
      };

      union {
	struct {
	uint8_t:1; // unused
	  bool irqFlags_crcOk:1; 
	  bool irqFlags_payloadReady:1; 
	  bool irqFlags_packetSent:1; 
	  bool irqFlags_fifoOverrun:1; 
	  bool irqFlags_fifoLevel:1; 
	  bool irqFlags_fifoNotEmpty:1; 
	  bool irqFlags_fifoFull:1; 
 	};
	uint8_t irqFlags2;
      };
      
      uint8_t rssiThresh;
      uint8_t rxTimeout1_rxStart;
      uint8_t rxTimeout2_rssiThresh;
      uint16_t preambleSize; // big endian

      uint8_t syncConfig_tol:3;
      uint8_t syncConfig_size:3;
      FifoFillCondition syncConfig_fifoFillCondition:1;
      bool syncConfig_syncOn:1;

      uint64_t syncValue; // beware BIG ENDIAN
      
    uint8_t:1; // unused
      AddressFiltering packetConfig_addressFiltering:2;
      bool packetConfig_crcAutoClearOff:1;
      bool packetConfig_crcOn:1;
      DCFree packetConfig_dcFree:2;
      PacketFormat packetConfig_packetFormat:1;

      uint8_t payloadLength;
      uint8_t nodeAddress;
      uint8_t broadcastAddress;
      
      IntermediateMode autoModes_intermediateMode:2;
      ExitCondition autoModes_exitCondition:3;
      EnterCondition autoModes_enterCondition:3;

      uint8_t fifoThresh_threshold:7;
      TxStartCondition fifoThresh_txStartCondition: 1;
      
      bool packetConfig2_aesOn:1;
      bool packetConfig2_autoRxRestartOn:1;
      bool packetConfig2_restartRx:1;
    uint8_t:1; // unused
      uint8_t packetConfig2_interPacketRxDelay:4;


      
      std::array<uint8_t, 16> aesKey; // MSB first

    uint8_t:2; // unused
      bool temp_measureRunning:1;
      bool temp_measureStart:1;
    uint8_t:4; // unused
      
      uint8_t temp_value;
      
    uint64_t:64;   // unused
      TestLna testLna_sensitivityBoost:8;
    uint8_t:8; // unused
      TestPa1 testPa1;
    uint8_t:8; // unused
      TestPa2 testPa2;
    uint64_t:64;   // unused
    uint64_t:64;   // unused
    uint16_t:16;   // unused
      FadingMargin testDagc;
    uint8_t:8; // unused
      uint8_t testAfc;
    } __attribute__((packed));
    uint8_t raw[0x72];
  };
};

static_assert(offsetof(Rfm69Rmap, testAfc) == 0x71);
static_assert(sizeof(Rfm69Rmap) == 0x72);
static_assert(static_cast<uint8_t>(ListenResolution::MS_262) == 0b11);
static_assert(static_cast<uint8_t>(ListenEnd::RESUME_IDLE) == 0b10);
