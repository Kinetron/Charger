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

#define NUMBER_OF_MEASUREMENTS 15
#define MAX_DELTA 150

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

//Floating average
uint32_t timerPeriodValues[NUMBER_OF_MEASUREMENTS];
uint8_t timerPeriodInterrupts[NUMBER_OF_MEASUREMENTS];
uint8_t measurementCounter = 0; // 0 - (NUMBER_OF_MEASUREMENTS - 1)

uint32_t timerPeriodValues_temp[NUMBER_OF_MEASUREMENTS];
uint8_t timerPeriodInterrupts_temp[NUMBER_OF_MEASUREMENTS];

uint32_t fullPeriodArr[NUMBER_OF_MEASUREMENTS];
uint32_t diffPeriodArr[NUMBER_OF_MEASUREMENTS];

uint8_t i = 0, j =0; 
uint8_t windowSize = 0; 
uint64_t periodSum = 0;
uint64_t diffSum = 0;

uint32_t temp = 0;


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
    // Setting the default state.  
    if ( HAL_GPIO_ReadPin( USER_LED_GPIO_Port, USER_LED_Pin ) == GPIO_PIN_SET )
    {
        HAL_GPIO_WritePin( USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET );
    }

    ssd1306_Init();
    HAL_IWDG_Refresh(&hiwdg);
}

uint32_t calculatingDiffModule(uint32_t current, uint32_t other)
{
     if(current > other)
     {
       return current - other;
     }
     else
     {
       return other - current;
     }
}

uint32_t calcPeriod(uint32_t interrupts, uint32_t remains)
{
   return 0xFFFF * interrupts + remains;
}

void calculatingAverage()
{
    if(measuredPeriodReady)
    {
       //Fast copy main to temp array.
       memcpy(timerPeriodValues_temp, timerPeriodValues, 4 * NUMBER_OF_MEASUREMENTS); 
       memcpy(timerPeriodInterrupts_temp, timerPeriodInterrupts, 1 * NUMBER_OF_MEASUREMENTS); 

       //calculation of the period 
       for(i = 0; i < NUMBER_OF_MEASUREMENTS; i++)
       {
         //fullPeriodArr[i] = calcPeriod(timerPeriodInterrupts_temp[i], timerPeriodValues_temp[i]);
         fullPeriodArr[i] = calcPeriod(timerPeriodInterrupts[i], timerPeriodValues[i]);
       } 

      //calculation diff of the period. artificial delay.
      for(i = 0; i < NUMBER_OF_MEASUREMENTS; i++)
      {
        diffSum = 0;
        for(j = 0; j < NUMBER_OF_MEASUREMENTS; j++)
        {
          diffSum+= (uint64_t)calculatingDiffModule(fullPeriodArr[i], fullPeriodArr[j]); //Sum
        }

        //average 
        diffPeriodArr[i] = (uint32_t)(diffSum / (NUMBER_OF_MEASUREMENTS - 1));
      }  

/*
      fullPeriod = fullPeriodArr[0];
      measuredPeriodReady = false; 

      resultFrequency = 0;

     if(fullPeriod > 0xFFFF)
      {         
        resultFrequency = ((float)70000000 / ((float)fullPeriod / 8 * 2));// - 0.0037;// - (59.7883 + 0.0318);//; //error clock tim2 35mhz 
       
        memcpy(ModbusRegister, &resultFrequency, sizeof(float)); //Convert to array.
        //memcpy(&ModbusRegister[2], &fullPeriodArr[0], 4); //Convert to array.
      }  

  measuredPeriodReady = false; 
       return;
       */

      HAL_IWDG_Refresh(&hiwdg);
      
      windowSize = NUMBER_OF_MEASUREMENTS;
      periodSum = 0;
      for(i = 0; i < NUMBER_OF_MEASUREMENTS; i++)
      {
          //Good values. 
          if(diffPeriodArr[i] < MAX_DELTA)
          {
            periodSum+= fullPeriodArr[i];  
            continue;
          }

          windowSize --;
      }  
      
      fullPeriod = periodSum / windowSize;
      measuredPeriodReady = false; 

      resultFrequency = 0;

      if(fullPeriod > 0xFFFF)
      {         
        resultFrequency = ((float)70000000 / ((float)fullPeriod / 8 * 2));// - 0.0037;// - (59.7883 + 0.0318);//; //error clock tim2 35mhz 
       
        memcpy(ModbusRegister, &resultFrequency, sizeof(float)); //Convert to array.
        memcpy(&ModbusRegister[3], &fullPeriodArr[0], 4); //Convert to array.
        memcpy(&ModbusRegister[5], &fullPeriodArr[1], 4); //Convert to array.
        memcpy(&ModbusRegister[7], &fullPeriodArr[2], 4); //Convert to array.
        memcpy(&ModbusRegister[9], &windowSize, 1); //Convert to array.
      }  

      HAL_IWDG_Refresh(&hiwdg); 
    }  
}

/**
 * \brief   It is performed periodically in the body of the main loop.
 *
 */
void loop( void )
{   
    HAL_IWDG_Refresh(&hiwdg);
    uartDataHandler();
    
    calculatingAverage();
 
    //Show on display.
    if(secondTimerHandler == true)
    {
      HAL_IWDG_Refresh(&hiwdg);    

      if(fullPeriod < 0xFFFF)
      {
         memcpy(ModbusRegister, &resultFrequency, sizeof(float)); //Convert to array.
      }

      //PB8 - input turn off the display.
      if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_RESET)
      {
        ssd1306_Fill(Black);
        ssd1306_UpdateScreen();
        return;
      }


      ssd1306_Fill(Black);
      ssd1306_SetCursor(0, 20);  
            
      sprintf(displayStr, "%10.7lf", resultFrequency);
 
      ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White);
      ssd1306_UpdateScreen();

      ssd1306_SetCursor(0, 40);
      sprintf(displayStr, "%d", fullPeriod);  //period
      ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White);
      ssd1306_UpdateScreen();
/*
      ssd1306_SetCursor(0, 0);
      sprintf(displayStr, "%d", tim2InterruptsCurrent); // bad param
      ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White);
      ssd1306_UpdateScreen();*/
    
      HAL_IWDG_Refresh(&hiwdg);
      
      fullPeriod = 0; //Clear value.
      tim2InterruptsCurrent = 0;
      resultFrequency = 0;
      secondTimerHandler = false;      
    }   
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
          
            timerPeriodValues[measurementCounter] = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_3);
            timerPeriodInterrupts[measurementCounter] = tim2InterruptsCounter;

            if(measurementCounter < NUMBER_OF_MEASUREMENTS - 1)
            {              
              measurementCounter ++;
            }
            else
            {
              measurementCounter = 0;

              //Synchronization with the main program
              if(measuredPeriodReady == false) //The main program get the data
              {              
                measuredPeriodReady = true;              
              }
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
}