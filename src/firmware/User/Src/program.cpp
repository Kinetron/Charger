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


//#define USE_DISPLAY
#define NUMBER_OF_MEASUREMENTS 15
#define MAX_DELTA 400

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

extern uint16_t ModbusRegister[NUMBER_OF_REGISTER];
extern uint16_t AnalogOutputHoldingRegister[NUMBER_OF_ANALOG_REGISTER];
extern bool ModbusCoil[NUMBER_OF_COIL];

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

uint32_t numberOfsignalMeasure = 0; //increases if there is a signal, reset every second.
uint32_t lastNumberOfsignalMeasure = 0;
bool deviceStartup = false; 


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
    //Delay 1 second.
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); //White LED on.
    HAL_IWDG_Refresh(&hiwdg);
    HAL_Delay(500);
    HAL_IWDG_Refresh(&hiwdg);
    HAL_Delay(500);
    HAL_IWDG_Refresh(&hiwdg);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); //White LED off.
     
    #ifdef USE_DISPLAY
     ssd1306_Init(); 
    #endif 
    
    HAL_IWDG_Refresh(&hiwdg);
    deviceStartup = true;

    AnalogOutputHoldingRegister[0] = MAX_DELTA;
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
    if(measuredPeriodReady) //synchronization with the timer2
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
        diffPeriodArr[i] = (uint32_t)(diffSum / (NUMBER_OF_MEASUREMENTS));
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
          if(diffPeriodArr[i] < AnalogOutputHoldingRegister[0])
          {
            periodSum+= fullPeriodArr[i];  
            continue;
          }

          windowSize --;
      }  
      
      //The delta is too small.
      if(windowSize == 0)
      {
        periodSum= fullPeriodArr[0];  
        windowSize = 1;
      }

      fullPeriod = periodSum / windowSize;

      if(fullPeriod > 0) //so that there is no division by zero
      {          
        //resultFrequency = ((float)70000000 / ((float)fullPeriod / 8 * 2));  
        resultFrequency = ((float)35000000 / (((float)fullPeriod) / 8));       
      }  

      measuredPeriodReady = false; 
      HAL_IWDG_Refresh(&hiwdg); 
    }  
}

void resetFrequencyData()
{
  if(deviceStartup == true & (lastNumberOfsignalMeasure == numberOfsignalMeasure))
  {
    fullPeriod = 0; //Clear value.
    resultFrequency = 0;
    memset(fullPeriodArr, 0, sizeof(fullPeriodArr)); 
    windowSize = 0;   
  }
  else
  {
    if(numberOfsignalMeasure > 0xFFFFF) 
    {
      numberOfsignalMeasure = 0;
    }

    lastNumberOfsignalMeasure = numberOfsignalMeasure;
  }
}

void updateOutputs()
{   
   if(ModbusCoil[0] == true)
   {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
   }
   else
   {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
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
    
    memcpy(ModbusRegister, &resultFrequency, sizeof(float)); //Convert to array.
    memcpy(&ModbusRegister[3], &fullPeriodArr[0], sizeof(uint32_t)); //Convert to array.
    memcpy(&ModbusRegister[5], &fullPeriodArr[1], sizeof(uint32_t)); //Convert to array.
    memcpy(&ModbusRegister[7], &fullPeriod, 4); //Convert to array.

    memcpy(&ModbusRegister[9], &windowSize, 1); //Convert to array.
    /* 
    memcpy(&ModbusRegister[9], &windowSize, 1); //Convert to array.
    */

    updateOutputs();
    
    //Second timer.
    if(secondTimerHandler == true)
    {
      HAL_IWDG_Refresh(&hiwdg);  
      
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);      
      
  #ifdef USE_DISPLAY
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
   #endif 

      HAL_IWDG_Refresh(&hiwdg);
      
      resetFrequencyData(); //Clear data if no signal. 
      
      secondTimerHandler = false;
      deviceStartup = true;      
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
            numberOfsignalMeasure++;                    
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