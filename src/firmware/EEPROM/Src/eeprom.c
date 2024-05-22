#include "eeprom.h"

void at24_WriteByte(uint16_t address, uint8_t *pData, uint8_t dataSize)
{
    while(HAL_I2C_Mem_Write(EEPROM_I2C, EEPROM_ADDR, 
    address, I2C_MEMADD_SIZE_8BIT, pData, (uint16_t)dataSize, 1000)!= HAL_OK)
    {

    }

    HAL_Delay(10);
}

void at24_ReadByte(uint16_t address, uint8_t *pData, uint8_t dataSize)
{
   //while(HAL_I2C_Mem_Write(hi2c,(uint16_t)DevAddress,(uint16_t)16,I2C_MEMADD_SIZE_8BIT,pData,(uint16_t)((MemAddress+TxBufferSize)-16),1000));
   if(HAL_I2C_Mem_Read(EEPROM_I2C, EEPROM_ADDR, 
    address, I2C_MEMADD_SIZE_8BIT, pData, (uint16_t)dataSize, 1000) != HAL_OK)
    {

    }     
}