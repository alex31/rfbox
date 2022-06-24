// Private configuration file for the SSD1306 driver.
#pragma once

//=== SSD1306 I2C Slave Address ===
#define SSD1306_I2C_ADDR		0x3C // 7 bits address (SA0 = 0)
//#define SSD1306_I2C_ADDR        0x3D // 7 bits address (SA0 = 1)

//=== SSD1306 width in pixels ===
// Some OLEDs don't display anything in first two columns.
// In this case change the following macro to 130.
#define SSD1306_WIDTH           128

//=== SSD1306 Display height ===
// OLED Display height in pixels: 32, 64 or 128.
#define SSD1306_HEIGHT			64

//=== Screen Mirroring ===
// uncomment if needed
//#define SSD1306_MIRROR_VERT
//#define SSD1306_MIRROR_HORIZ

//=== Color Inversion ===
// uncomment if needed
// #define SSD1306_INVERSE_COLOR

