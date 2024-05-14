#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpio.h"
#include "usart.h"
#include "main.h"
#include "ssd1306.h"
#include "fee.h"
#include "soft_timers.h"
#include "stdio.h"

#define ADC_NUMBER_OF_CHANNELS 9 //Use 9 channels to measure parameters.
#define LED_USER_PERIOD_MSEC  ( 500 )
#define USART_STRING_SIZE 70
#define UART huart1
#define ADC_REFERENCE_VOLTAGE 3.3
#define ADC_MAX 0xFFF //Max adc value.
#define PARAM_STR_LEN 5

#define DISPLAY_VOLTAGE 0
#define DISPLAY_CURRENT 1
#define DISPLAY_CAPACITY 2
#define DISPLAY_PERCENTS 3
 
#define DIVISION_COEFFICIENTS_VOLTAGE 4.55
#define DIVISION_COEFFICIENTS_CURRENT 3.64
#define MAX_QUANTITY_MEASUREMENTS 15

extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim3;
extern ADC_HandleTypeDef hadc1;

volatile uint32_t TimeTickMs = 0;
uint32_t oldTimeTickHSec = 0;

uint32_t adcData[ADC_NUMBER_OF_CHANNELS]; //Measured adc values.

uint32_t adcAvgBuff[2];
float adcResults[2];
uint8_t numberMeasurements = 0;
float calculatedCapacity;

bool secondTimerHandler = false;
float lastCurrent;

char PassWord_W[10] = {"ABC123xyz"};
char PassWord_R[9];
bool logoSwith = false;

char displayStr[30]; //To display data on the screen.
bool animationFlag = false; //To flash the animation on the screen

char usartString[USART_STRING_SIZE]; //The string that will be sent via usart.

float actualVoltage;
float actualCurrent;
float actualCapacity;

/**
 * \brief  Performs initialization. 
 *
 */
void init( void )
{
   
}

/**
 * \brief  Performs additional settings.
 *
 */
void setup( void )
{
    HAL_ADC_Start_DMA(&hadc1, adcData, ADC_NUMBER_OF_CHANNELS); // start adc in DMA mode

    // Setting the default state.  
    if ( HAL_GPIO_ReadPin( USER_LED_GPIO_Port, USER_LED_Pin ) == GPIO_PIN_SET )
    {
        HAL_GPIO_WritePin( USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET );
    }
    ssd1306_Init();

   HAL_IWDG_Refresh(&hiwdg);
}

float calculatedDC(uint32_t adcValue, bool paramType)
{
   if(paramType)
   {
      return adcValue  * ADC_REFERENCE_VOLTAGE * DIVISION_COEFFICIENTS_VOLTAGE / ADC_MAX;
   }
   else
   {
      return adcValue  * ADC_REFERENCE_VOLTAGE * DIVISION_COEFFICIENTS_CURRENT / ADC_MAX;
   }
}

void toFloatStr(float data, char* str)
{
  sprintf(str, "%d.%02d", (uint32_t)data, (uint16_t)((data - (uint32_t)data) * 100.)); 
}

//Show on display the current or voltage
void printDisplayParameter(float data, uint8_t paramType)
{
   toFloatStr(data, displayStr);

   char* simvol = " В";

   switch (paramType)
   {
    case DISPLAY_VOLTAGE: 
      ssd1306_SetCursor(0, 18);
    break;
   
    case DISPLAY_CURRENT:
      ssd1306_SetCursor(80, 18);
      simvol = " A";
    break;

    case DISPLAY_CAPACITY: 
      ssd1306_SetCursor(0, 0);
      simvol = " Ah";
    break;

    case DISPLAY_PERCENTS:
      sprintf(displayStr,"%d ", (uint8_t)data);
      ssd1306_SetCursor(85, 0);
      
      if(animationFlag)
      {
        simvol = " % ";  
      }
      else
      {
        simvol = "% ";
      }
      animationFlag = !animationFlag;
    break; 
 
   default:
    break;
   }
  
   ssd1306_PrintString(displayStr, 2);
   ssd1306_PrintString(simvol, 2);
}

char tmpStr[PARAM_STR_LEN];
void sendParametersToUsart(float voltage, float current, float capacity)
{
  uint8_t lastPos = 0;
  
  usartString[0] = '\0';
  toFloatStr(voltage, usartString);
  lastPos = strlen(usartString);

  strcpy(usartString + lastPos, " V ");
  lastPos = strlen(usartString); 

  toFloatStr(current, usartString + lastPos);
  lastPos = strlen(usartString);

  strcpy(usartString + lastPos, " A ");
  lastPos= strlen(usartString);

  toFloatStr(capacity, usartString + lastPos);
  lastPos= strlen(usartString);;

  strcpy(usartString + lastPos, " Ah\n");
  
  HAL_IWDG_Refresh(&hiwdg);
  HAL_UART_Transmit( & UART, (uint8_t *) usartString, sizeof(usartString), 50 );
}

void calculateCapacity()
{
   if(secondTickHandler)
   {
     secondTimerHandler = false;
     calculatedCapacity+= calculatedDC(adcResults[1], false);
   }
}

/**
 * \brief   It is performed periodically in the body of the main loop.
 *
 */

void loop( void )
{
    HAL_Delay(1000);

    HAL_IWDG_Refresh(&hiwdg);

    //ssd1306_SetCursor(0,0);
    //ssd1306_PrintString("ЗАПУСК..", 2);
    calculateCapacity(); 
     
    ssd1306_Fill(Black);
    actualVoltage = calculatedDC(adcResults[0], true);
    actualCurrent = calculatedDC(adcResults[1], false);
    actualCapacity = calculatedCapacity / 3600;
    printDisplayParameter(actualVoltage, DISPLAY_VOLTAGE);
    printDisplayParameter(actualCurrent, DISPLAY_CURRENT); 
    printDisplayParameter(actualCapacity, DISPLAY_CAPACITY); 
    printDisplayParameter(100, DISPLAY_PERCENTS);  
    ssd1306_UpdateScreen();

    // We are waiting for the end of the packet transmission.
    if (UART.gState != HAL_UART_STATE_READY ) return;
    sendParametersToUsart(actualVoltage, actualCurrent, actualCapacity);

}

void HAL_SYSTICK_Callback( void )
{
    TimeTickMs++;

    if (TimeTickMs - oldTimeTickHSec > 1000)
    {
        oldTimeTickHSec = TimeTickMs;
        secondTimerHandler = true;
        lastCurrent = adcResults[1];        
    }    
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{   
   if (htim->Instance == TIM2)
   {      
      if(numberMeasurements < MAX_QUANTITY_MEASUREMENTS)
      {
         for(uint8_t i = 0; i < 2; i++)
         {
           adcAvgBuff[i]+= adcData[i]; 
         }

         numberMeasurements++;    
      }
      else
      {
        for(uint8_t i = 0; i < 2; i++)
         {
           adcResults[i]= adcAvgBuff[i] / numberMeasurements; 
           adcAvgBuff[i] = 0;
         }

         numberMeasurements = 0;
      }
   }
}