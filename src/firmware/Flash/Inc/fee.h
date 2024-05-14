/*
 * File: FEE.h
 * Driver Name: [[ FEE (Flash EEPROM Emulation) ]]
 * SW Layer:   MIDWR
 * Author:     Khaled Magdy
 * -------------------------------------------
 * For More Information, Tutorials, etc.
 * Visit Website: www.DeepBlueMbedded.com
 */

/*
 * Disclaimer: This library is a quick & dirty implementation for FLASH wrapper functions
 * That may be helpful for basic applications, not for use in anything serious obviously.
 */

#define FLASH_WRP1_WRP1
#include "main.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>  // For memcpy
#include "stm32f1xx_hal_conf.h"



#define FEE_START_ADDRESS	((uint32_t)0x08007C00)  // FEE Start Address (in Flash Memory)
#define FEE_PAGE_SIZE		((uint32_t)0x400)       // FEE Page Size = 1kB (Default Page Size For STM32F103)
#define FEE_BUFFER_LEN		(FEE_PAGE_SIZE / 2U)

_BEGIN_STD_C

bool test(void);
HAL_StatusTypeDef FEE_Write(uint32_t address, uint16_t *data, uint32_t dataSize);
void FEE_Read(uint32_t address, uint16_t *data, uint32_t dataSize);

// Generic APIs For Any Data Type
void FEE_WriteData(uint32_t address, void *data, size_t dataSize);
void FEE_ReadData(uint32_t address, void *data, size_t dataSize);

_END_STD_C


