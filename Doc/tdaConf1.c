// ************************************************************************************
// C source file For TDA5150 configuration purposes
// (c) Infineon AG, 2006
// ************************************************************************************

static const unsigned char regVals[] = { 0x00, 0x00, 0x00, 0x00, 0x06, 0x25, 0x12, 0xA1, 
                                         0xAB, 0x21, 0x7E, 0x5D, 0x0C, 0x40, 0x00, 0x00, 
                                         0x10, 0x40, 0x00, 0x00, 0x10, 0x40, 0x00, 0x00, 
                                         0x10, 0x00, 0xFC, 0xBB, 0xDE, 0x51, 0x48, 0x20, 
                                         0x4C, 0x0B, 0x41, 0x00, 0x24, 0x58, 0xC0, 0x00 };


void WriteRegister(unsigned char address, unsigned char regVal)
{
    /* TODO: Write code to serve SPI here */
}

void ConfigureTda5150()
{
    unsigned char address;

    for (address = 0x04; address <= 0x27; address++)
    {
        WriteRegister(address, regVals[address]);
    }
}

