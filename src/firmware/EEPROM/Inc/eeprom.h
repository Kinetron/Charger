#include "main.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>  // For memcpy
#include "stm32f1xx_hal_conf.h"

extern I2C_HandleTypeDef hi2c1;
#define EEPROM_I2C &hi2c1
#define EEPROM_TIMOUT 300
#define EEPROM_WRITE_TIME 15 //Self-timed Write Cycle (5 ms max)

 //In our example, we connect A0, A1, A2 directly to VCC in this way the device address is 1010111 (in general A0, A1, A2 identify the last three significant bits of the device address 
 //1 0 1 0 A2 A1 A0) is 0x57 in Hexadecimal. The 4 most significant bits are preset (Control Code),
 // the A0, A1, A2 are Chip Select Bits.
//AT24C08A, 8K SERIAL EEPROM: Internally organized with 64 pages of 16 bytes
//each, the 8K requires a 10-bit data word address for random word addressing. 

// EEPROM ADDRESS (8bits)
#define EEPROM_ADDR 0x50 << 1

// Define the Page Size and number of pages 
#define PAGE_SIZE 128     // in Bytes
#define PAGE_NUM  64    // number of pages

_BEGIN_STD_C

bool at24_WriteByte(uint16_t address, uint8_t *pData, uint8_t dataSize);
bool at24_ReadByte(uint16_t address, uint8_t *pData, uint8_t dataSize);

_END_STD_C