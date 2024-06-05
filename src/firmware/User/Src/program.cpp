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
#include "ssd1306_fonts.h"
#include "eeprom.h"

#define TEMP_SENSOR_VOLT_25   1.31f      // The voltage (in volts) on the sensor at a temperature of 25 °C.
#define TEMP_SENSOR_SLOPE  0.0043f    // Voltage change (in volts) when the temperature changes by a degree.
#define ADC_NUMBER_OF_CHANNELS 4 //Use 9 channels to measure parameters.
#define LED_USER_PERIOD_MSEC  ( 500 )
#define USART_STRING_SIZE 100
#define UART huart1
#define ADC_REFERENCE_VOLTAGE 33 //*10
#define ADC_MAX 0xFFF //Max adc value.
#define PARAM_STR_LEN 5

#define DISPLAY_VOLTAGE 0
#define DISPLAY_CURRENT 1
#define DISPLAY_CAPACITY 2
#define DISPLAY_PERCENTS 3
 
//100k +24k  5v in 0.97v 600adc 
#define DIVISION_COEFFICIENTS_VOLTAGE 5.309//5.498 -- 5v 
#define DIVISION_COEFFICIENTS_CURRENT 3.64
#define MAX_QUANTITY_MEASUREMENTS 16

#define MAX_VOLTAGE 14.0
#define MIN_VOLTAGE 10.0

extern IWDG_HandleTypeDef hiwdg;
extern ADC_HandleTypeDef hadc1;

volatile uint32_t TimeTickMs = 0;
uint32_t oldTimeTickHSec = 0;

uint32_t adcData[ADC_NUMBER_OF_CHANNELS]; //Measured adc values.

uint32_t adcAvgBuff[ADC_NUMBER_OF_CHANNELS];
uint32_t adcResults[ADC_NUMBER_OF_CHANNELS];
uint8_t numberMeasurements = 0;
float calculatedCapacity;

bool secondTimerHandler = false;
float lastCurrent;

char PassWord_W[10] = {"ABC123xyz"};
char PassWord_R[9];
bool logoSwith = false;

char displayStr[300]; //To display data on the screen.
bool animationFlag = false; //To flash the animation on the screen

char usartString[USART_STRING_SIZE]; //The string that will be sent via usart.

float actualVoltage;
float actualCurrent;
float actualCapacity;

uint32_t period = 0;
uint32_t pulseWidth = 0;
uint32_t tim2InterruptsCounter = 0;
uint32_t tim2InterruptsCurrent = 0;

extern TIM_HandleTypeDef htim2;

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
    //HAL_ADC_Start_DMA(&hadc1, adcData, ADC_NUMBER_OF_CHANNELS); // start adc in DMA mode

    // Setting the default state.  
    if ( HAL_GPIO_ReadPin( USER_LED_GPIO_Port, USER_LED_Pin ) == GPIO_PIN_SET )
    {
        HAL_GPIO_WritePin( USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET );
    }
    ssd1306_Init();

    HAL_IWDG_Refresh(&hiwdg);
  
    ssd1306_SetCursor(0, 20); 
    ssd1306_PrintString("Привет", 2);
    ssd1306_UpdateScreen();

    return;
    const int mem_size = 100;
    uint8_t arr[mem_size];

    uint8_t data = 0;
    for(int i = 0; i < mem_size; i++)
    {      
      if(!at24_WriteByte(data, &data, 1)) return;
      HAL_IWDG_Refresh(&hiwdg);
      data++;
    }

    at24_ReadByte(0, arr, mem_size);
    for(int i = 0; i < mem_size; i++)
    {
       HAL_IWDG_Refresh(&hiwdg);
       sprintf(displayStr,"%d ", arr[i]);
       ssd1306_SetCursor(0, 0);
       ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White); 
       ssd1306_SetCursor(0, 20);
       if(i % 2 == 0)
       {
          ssd1306_WriteSpecialSimvolString("1", SpecialCharacters_11x18, White);
       }
       else
       {
         ssd1306_WriteSpecialSimvolString("0", SpecialCharacters_11x18, White);
       }
       ssd1306_UpdateScreen();
      
       HAL_Delay(500);
    }

}

float calculatedDC(uint32_t adcValue, bool paramType)
{
   if(paramType)
   {
       return  adcValue * ADC_REFERENCE_VOLTAGE * DIVISION_COEFFICIENTS_VOLTAGE / 40960;
   }
   else
   {
      return adcValue *  ADC_REFERENCE_VOLTAGE / 4096;
   }
}

float calculatedTemperature(uint32_t adcValue)
{
  //When heated, the voltage down.
  float voltage = (float) adcValue / 4096 * ADC_REFERENCE_VOLTAGE;   //The voltage in volts on the sensor.
  //For Calibrated
  return voltage;
  //return ((TEMP_SENSOR_VOLT_25 - voltage) / TEMP_SENSOR_SLOPE)  + 25;   // Temperature in degrees.
}

void toFloatStr(float data, char* str)
{
  sprintf(str, "%d.%02d", (uint32_t)data, (uint16_t)((data - (uint32_t)data) * 100.)); 
}

void toFloatStrShort(float data, char* str)
{
  sprintf(str, "%d.%d", (uint32_t)data, (uint16_t)((data - (uint32_t)data) * 10.)); 
}

char * percentAnimation(float value)
{
    char *simvol = "";
    bool maxPersents = false;
    if((uint8_t)value == 100) maxPersents = true;

    if(animationFlag)
      {
        simvol =  " % ";  
        if(maxPersents) simvol = 0;
      }
      else
      {
        simvol = "% ";
      }

      animationFlag = !animationFlag;

      return simvol;
} 

//Show on display the current or voltage
void printDisplayParameter(float data, uint8_t paramType, bool shortFormat)
{
   if(shortFormat)
   {
      toFloatStrShort(data, displayStr);
   }
   else
   {
     toFloatStr(data, displayStr);
   }
   

   char* simvol = "B";

   switch (paramType)
   {
    case DISPLAY_VOLTAGE: 
      ssd1306_SetCursor(0, 45);
    break;
   
    case DISPLAY_CURRENT:
      ssd1306_SetCursor(75, 45);
      simvol = "A";
    break;

    case DISPLAY_CAPACITY: 
      ssd1306_SetCursor(0, 18);
      simvol = "Ah";
    break;

    case DISPLAY_PERCENTS:
      sprintf(displayStr,"%d ", (uint8_t)data);
      ssd1306_SetCursor(85, 18);
      
      simvol = percentAnimation(data);
    break; 
 
   default:
    break;
   }
  
   if(simvol != 0)
   {
       //For string with 3digits.
      if((strlen(displayStr) < 4) && paramType != DISPLAY_PERCENTS)
      {
        ssd1306_MoveCursor(7, 0);
      }    
      
      //ssd1306_WriteString(displayStr, Font_11x18, White);
      ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White); 
   }
   else
   {
    //ssd1306_PrintString("", 2);
    //ssd1306_WriteString("", Font_11x18, White);
    ssd1306_WriteSpecialSimvolString(simvol, SpecialCharacters_11x18, White); 
   }
  
   ssd1306_MoveCursor(0, 7);
   ssd1306_WriteSpecialSimvolString(simvol, SpecialCharacters_7x10, White);    
}

void sendFreqToUsart(double frequency)
{
  sprintf(usartString, "%10.7lf\r\n", frequency);

  HAL_IWDG_Refresh(&hiwdg);
  HAL_UART_Transmit( & UART, (uint8_t *) usartString, sizeof(usartString), 50 );
}

void sendParametersToUsart(uint32_t rawAdc, float voltage, float current, float capacity, float temperature)
{
     strcpy(usartString, "t");

  HAL_IWDG_Refresh(&hiwdg);
  HAL_UART_Transmit( & UART, (uint8_t *) usartString, sizeof(usartString), 50 );

  return;

  uint8_t lastPos = 0;
  
  usartString[0] = '\0';
  sprintf(usartString, "%d.%02d", rawAdc); 
  lastPos = strlen(usartString);
  strcpy(usartString + lastPos, " ");
  lastPos = strlen(usartString);

  toFloatStr(voltage, usartString + lastPos);
  lastPos = strlen(usartString);

  strcpy(usartString + lastPos, " V ");
  lastPos = strlen(usartString); 

  toFloatStr(current, usartString + lastPos);
  lastPos = strlen(usartString);

  strcpy(usartString + lastPos, " A ");
  lastPos= strlen(usartString);

  toFloatStr(capacity, usartString + lastPos);
  lastPos= strlen(usartString);

  strcpy(usartString + lastPos, " Ah ");

  lastPos= strlen(usartString);
  toFloatStr(temperature, usartString + lastPos);
  lastPos= strlen(usartString);
  strcpy(usartString + lastPos, " t\n");

  HAL_IWDG_Refresh(&hiwdg);
  HAL_UART_Transmit( & UART, (uint8_t *) usartString, sizeof(usartString), 50 );
}

void calculateCapacity()
{
   if(secondTickHandler)
   {
     secondTimerHandler = false;
     calculatedCapacity+= calculatedDC(adcResults[0], false);
   }
}

float calculatePercents(float voltage)
{
   float percents = (voltage - MIN_VOLTAGE) / ((MAX_VOLTAGE - MIN_VOLTAGE) / 100.0);
   if((uint8_t)percents > 100) return 100.0;

   return percents;
}

/**
 * \brief   It is performed periodically in the body of the main loop.
 *
 */

int contrast = 10;
void loop( void )
{   
    HAL_IWDG_Refresh(&hiwdg);
    HAL_Delay(1000);
    //calculateCapacity(); 
    HAL_IWDG_Refresh(&hiwdg);
    ssd1306_Fill(Black);

   ssd1306_SetCursor(0, 20);
   
   uint32_t fullPeriod = 0xFFFF * tim2InterruptsCurrent + period;
   
   /*
     Use quartz generator 119.6063 Hz
     The quartz resonator has an incorrect frequency. Is not 10 1000.
     Now TIM2 count 35mhz (10*3.5)
     Check errors 59.8042 / 17.0867 = 3,500043893788736;
   */
   //double frequency = ((double)35000000 / (((double)fullPeriod) / 8)) - 17.0867; //error quarth 10mhz
   //double frequency = ((double)35000000 / (double)fullPeriod) - 59.8042;//; //error clock tim2 35mhz

   double frequency = ((double)35000000 / ((double)fullPeriod / 2)) - (59.7883 + 0.0318);//; //error clock tim2 35mhz
   //Bad double frequency = ((double)70000000 / ((double)fullPeriod / 2)) - 59.7883;//; //error clock tim2 35mhz
   sprintf(displayStr, "%10.7lf", frequency);
 
   ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White);
   ssd1306_UpdateScreen();

    ssd1306_SetCursor(0, 40);
    sprintf(displayStr, "%d", fullPeriod);  //period
    ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White);
    ssd1306_UpdateScreen();

    ssd1306_SetCursor(0, 0);
    sprintf(displayStr, "%d", tim2InterruptsCurrent); // bad param
    ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White);
    ssd1306_UpdateScreen();
 
 /*
    ssd1306_SetCursor(0, 40);
    sprintf(displayStr, "%d", pulseWidth); // bad param
    ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White);
    ssd1306_UpdateScreen();
*/
   
    HAL_IWDG_Refresh(&hiwdg); 
    // We are waiting for the end of the packet transmission.
    if (UART.gState != HAL_UART_STATE_READY ) return;
    sendFreqToUsart(frequency);
    //sendParametersToUsart(adcResults[1] ,actualVoltage, actualCurrent, actualCapacity, 1.23);

   return;
//uint32_t period = 0;
//uint32_t pulseWidth = 0; 


    actualVoltage = calculatedDC(adcResults[1], true);
    actualCurrent = calculatedDC(DIVISION_COEFFICIENTS_VOLTAGE * ADC_REFERENCE_VOLTAGE, false);
    actualCapacity = calculatedCapacity / 3600;
    float actualTemperature = calculatedTemperature(adcData[3]);
     
    //printDisplayParameter(actualTemperature, DISPLAY_VOLTAGE, false);
    printDisplayParameter(actualVoltage, DISPLAY_VOLTAGE, false);

    printDisplayParameter(actualCurrent, DISPLAY_CURRENT, true); 
    printDisplayParameter(actualCapacity, DISPLAY_CAPACITY, false); 
    printDisplayParameter(calculatePercents(actualVoltage), DISPLAY_PERCENTS, false); 

    ssd1306_UpdateScreen();

    HAL_IWDG_Refresh(&hiwdg); 
    // We are waiting for the end of the packet transmission.
    if (UART.gState != HAL_UART_STATE_READY ) return;
    sendParametersToUsart(adcResults[1] ,actualVoltage, actualCurrent, actualCapacity, actualTemperature);
      
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


void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
        {
            TIM2->CNT = 0;
            tim2InterruptsCurrent = tim2InterruptsCounter;
            tim2InterruptsCounter = 0;
        
            period = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_3);
            pulseWidth = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_4);
        }
    }
}

uint32_t tim_test = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{   
 if (htim->Instance == TIM2)
 {  
    tim2InterruptsCounter ++;   
 }
 return;
 
 if (htim->Instance == TIM3)
   {      
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

      return;
      
      if(numberMeasurements < MAX_QUANTITY_MEASUREMENTS)
      {
         for(uint8_t i = 0; i < ADC_NUMBER_OF_CHANNELS; i++)
         {
           adcAvgBuff[i]+= adcData[i]; 
         }

         numberMeasurements++;    
      }
      else
      {
        for(uint8_t i = 0; i < ADC_NUMBER_OF_CHANNELS; i++)
         {
           adcResults[i]= adcAvgBuff[i] / numberMeasurements; 
           adcAvgBuff[i] = 0;
         }

         numberMeasurements = 0;
      }

      uint32_t persent = (uint32_t)(adcData[0] / (0xFFF / 100));
      uint32_t pwm = 10 * persent;    
      TIM2->CCR3 = pwm;//tim_test;
      TIM2->CCR4 = 1000 - pwm;//tim_test;

      tim_test++;

      if(tim_test > 0xFFFFF) tim_test = 0;
   }
}