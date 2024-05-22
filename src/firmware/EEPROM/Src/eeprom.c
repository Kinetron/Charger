#include "eeprom.h"

bool at24_WriteByte(uint16_t address, uint8_t *pData, uint8_t dataSize)
{
    if(HAL_I2C_Mem_Write(EEPROM_I2C, EEPROM_ADDR, address, I2C_MEMADD_SIZE_8BIT, pData, (uint16_t)dataSize, EEPROM_TIMOUT) != HAL_OK)
    {
       return false;      
    }

    HAL_Delay(EEPROM_WRITE_TIME);

    return true;
}

bool at24_ReadByte(uint16_t address, uint8_t *pData, uint8_t dataSize)
{
   if(HAL_I2C_Mem_Read(EEPROM_I2C, EEPROM_ADDR, address, I2C_MEMADD_SIZE_8BIT, pData, (uint16_t)dataSize, EEPROM_TIMOUT) != HAL_OK)
   {
      return false; 
   }     

   return true;
}