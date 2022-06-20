#ifndef __TDA5150__
#define __TDA5150__

/**************************************/
/* TDA5150 Definitions                */
/**************************************/

/* Register Names & Addresses */

#define TXSTAT         0x01
#define VACRES         0x02
#define VACDIFF        0x03
#define TXCFG0         0x04
#define TXCFG1         0x05
#define CLKOUTCFG      0x06
#define BDRDIV         0x07
#define PRBS           0x08

#define PLLINTA        0x09
#define PLLFRACA0      0x0A
#define PLLFRACA1      0x0B
#define PLLFRACA2      0x0C
#define PLLINTB        0x0D
#define PLLFRACB0      0x0E
#define PLLFRACB1      0x0F
#define PLLFRACB2      0x10
#define PLLINTC        0x11
#define PLLFRACC0      0x12
#define PLLFRACC1      0x13
#define PLLFRACC2      0x14
#define PLLINTD        0x15
#define PLLFRACD0      0x16
#define PLLFRACD1      0x17
#define PLLFRACD2      0x18

#define SLOPEDIV       0x19
#define POWCFG0        0x1A
#define POWCFG1        0x1B
#define FDEV           0x1C
#define GFDIV          0x1D
#define GFXOSC         0x1E
#define ANTTDCC        0x1F
#define RES1           0x20
#define VAC0           0x21
#define VAC1           0x22
#define VACERRTH       0x23
#define CPCFG          0x24
#define PLLBW          0x25
#define RES2           0x26
#define ENCCNT         0x27

/* Register Values */

#define VAL_TXCFG0     0x06
#define VAL_TXCFG1     0x25
#define VAL_CLKOUTCFG  0x12
#define VAL_BDRDIV     0xA1
#define VAL_PRBS       0xAB

#define VAL_PLLINTA    0x21
#define VAL_PLLFRACA0  0x7E
#define VAL_PLLFRACA1  0x5D
#define VAL_PLLFRACA2  0x0C
#define VAL_PLLINTB    0x40
#define VAL_PLLFRACB0  0x00
#define VAL_PLLFRACB1  0x00
#define VAL_PLLFRACB2  0x10
#define VAL_PLLINTC    0x40
#define VAL_PLLFRACC0  0x00
#define VAL_PLLFRACC1  0x00
#define VAL_PLLFRACC2  0x10
#define VAL_PLLINTD    0x40
#define VAL_PLLFRACD0  0x00
#define VAL_PLLFRACD1  0x00
#define VAL_PLLFRACD2  0x10

#define VAL_SLOPEDIV   0x00
#define VAL_POWCFG0    0xFC
#define VAL_POWCFG1    0xBB
#define VAL_FDEV       0xDE
#define VAL_GFDIV      0x51
#define VAL_GFXOSC     0x48
#define VAL_ANTTDCC    0x20
#define VAL_RES1       0x4C
#define VAL_VAC0       0x0B
#define VAL_VAC1       0x41
#define VAL_VACERRTH   0x00
#define VAL_CPCFG      0x24
#define VAL_PLLBW      0x58
#define VAL_RES2       0xC0
#define VAL_ENCCNT     0x00


#endif
