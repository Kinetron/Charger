#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpio.h"
#include "usart.h"
#include "main.h"
#include "ssd1306.h"
#include "stdio.h"
#include "ssd1306_fonts.h"
#include "eeprom.h"
#include "ModbusRTU_Slave.h"

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

bool secondTimerHandler = false; //one second has passed
float lastCurrent;

char displayStr[300]; //To display data on the screen.

uint32_t period = 0;
uint32_t pulseWidth = 0;
uint32_t tim2InterruptsCounter = 0;
uint32_t tim2InterruptsCurrent = 0;

extern uint16_t ModbusRegister[NUMBER_OF_REGISTER];
extern TIM_HandleTypeDef htim2;

uint32_t fullPeriod = 0; //The calculated period of the last measurement.
bool measuredPeriodReady = false;//The measured period is ready
float resultFrequency = 0;

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
  
    for(int i = 0; i < NUMBER_OF_REGISTER; i++ )
    {
         ModbusRegister[i] = i;
    }
   
    return;
    /*
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
*/
}

/**
 * \brief   It is performed periodically in the body of the main loop.
 *
 */
void loop( void )
{   
    HAL_IWDG_Refresh(&hiwdg);
    uartDataHandler();
    
    if(measuredPeriodReady)
    {
       fullPeriod = 0xFFFF * tim2InterruptsCurrent + period;
       measuredPeriodReady = false;    
    }  

    //Show on display.
    if(secondTimerHandler == true)
    {
      HAL_IWDG_Refresh(&hiwdg);    

      ssd1306_Fill(Black);
      ssd1306_SetCursor(0, 20);

      resultFrequency = 151.25111112;//0;

      if(fullPeriod > 100)
      {         
        resultFrequency = ((float)70000000 / ((float)fullPeriod / 8 * 2)) - 0.0037;// - (59.7883 + 0.0318);//; //error clock tim2 35mhz 
      }

      memcpy(ModbusRegister, &resultFrequency, sizeof(float)); //Convert to array.      
      sprintf(displayStr, "%10.7lf", resultFrequency);
 
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
    
      HAL_IWDG_Refresh(&hiwdg); 

      fullPeriod = 0; //Clear value.
      secondTimerHandler = false;
    }

    return;
    HAL_Delay(1000);
    //calculateCapacity(); 
    
   
   
   //uint32_t fullPeriod = 0xFFFF * tim2InterruptsCurrent + period;
   //uint32_t fullPeriod = 70000000;
   /*
     Use quartz generator 119.6063 Hz
     The quartz resonator has an incorrect frequency. Is not 10 1000.
     Now TIM2 count 35mhz (10*3.5)
     Check errors 59.8042 / 17.0867 = 3,500043893788736;
   */
   //double frequency = ((double)35000000 / (((double)fullPeriod) / 8)) - 17.0867; //error quarth 10mhz
   //double frequency = ((double)35000000 / (double)fullPeriod) - 59.8042;//; //error clock tim2 35mhz


/*
   double frequency = ((double)70000000 / ((double)fullPeriod / 8 * 2)) - 0.0037;// - (59.7883 + 0.0318);//; //error clock tim2 35mhz
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
 
   
    HAL_IWDG_Refresh(&hiwdg); 
    */
    // We are waiting for the end of the packet transmission.
   // if (UART.gState != HAL_UART_STATE_READY ) return;
   // sendFreqToUsart(frequency);
    //sendParametersToUsart(adcResults[1] ,actualVoltage, actualCurrent, actualCapacity, 1.23);

   return;      
}

void HAL_SYSTICK_Callback( void )
{
    TimeTickMs++;
    if (TimeTickMs - oldTimeTickHSec > 1000)
    {
      oldTimeTickHSec = TimeTickMs;
      secondTimerHandler = true;
    }    
}


void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
        {
            TIM2->CNT = 0;

            //Synchronization with the main program
            if(measuredPeriodReady == false) //The main program get the data
            {
              //Update current values.
              tim2InterruptsCurrent = tim2InterruptsCounter;
              period = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_3);
              measuredPeriodReady = true;              
            }

            tim2InterruptsCounter = 0;                    
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{   
 if (htim->Instance == TIM2)
 {  
    tim2InterruptsCounter ++;   
 }
 return;
}