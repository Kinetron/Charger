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

extern bool hasInterrupt;

extern uint8_t DataCounter;
extern char ModbusRx[BUFFERSIZE];

extern char ModbusString[10];


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
    
    ssd1306_Init(); 
    
    
    HAL_IWDG_Refresh(&hiwdg);
    deviceStartup = true;

    AnalogOutputHoldingRegister[0] = MAX_DELTA;
}


/**
 * \brief   It is performed periodically in the body of the main loop.
 *
 */
void loop( void )
{   
    HAL_IWDG_Refresh(&hiwdg);
    uartDataHandler();
    
    
    //Second timer.
    if(secondTimerHandler == true)
    {
      HAL_IWDG_Refresh(&hiwdg);  
      
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);      
      
      ssd1306_Fill(Black);
      ssd1306_SetCursor(0, 20);  
            
      //sprintf(displayStr, "%d", ModbusRegister[0]);
 
      
      if(hasInterrupt)
      {
        ssd1306_WriteSpecialSimvolString("1", SpecialCharacters_11x18, White);
      }
      else
      {
        ssd1306_WriteSpecialSimvolString("-1", SpecialCharacters_11x18, White);
             // ModbusRegister[0] = 0;
      }

      hasInterrupt = false;

      ssd1306_SetCursor(0, 40);      


      //sprintf(displayStr, "%d", st);

      strcpy(displayStr, ModbusString);
      ssd1306_WriteSpecialSimvolString(displayStr, SpecialCharacters_11x18, White);
      ssd1306_UpdateScreen();
      HAL_Delay(500);
      HAL_IWDG_Refresh(&hiwdg);
       

      ssd1306_UpdateScreen();
      HAL_IWDG_Refresh(&hiwdg);
            
      secondTimerHandler = false;
      deviceStartup = true;      

    }   
}

void HAL_SYSTICK_Callback( void )
{
    uartTimer();

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