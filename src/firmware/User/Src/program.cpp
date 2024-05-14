#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpio.h"
#include "usart.h"
#include "main.h"
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "ssd1306_fonts.h"
#include "commands_decoder.h"
#include "fee.h"
#include "soft_timers.h"
#include "stdio.h"

#define ADC_NUMBER_OF_CHANNELS 9 //Use 9 channels to measure parameters.
#define LED_USER_PERIOD_MSEC  ( 500 )
#define USART_STRING_SIZE 70
#define UART huart1
#define ADC_REFERENCE_VOLTAGE 3.3
#define ADC_MAX 0xFFF //Max adc value.
#define VOLTAGE_STR_LEN 5

extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim3;
extern ADC_HandleTypeDef hadc1;

volatile uint32_t TimeTickMs = 0;
uint32_t oldTimeTickHSec = 0;

uint32_t adcData[ADC_NUMBER_OF_CHANNELS]; //Measured adc values. 

char PassWord_W[10] = {"ABC123xyz"};
char PassWord_R[9];
bool logoSwith = false;

// Options for the status of the transmission method.
enum TxState
{ 
  Default = 0, ///< synchronous transmission
  Interrupt, ///< asynchronous transmission (interrupts)
  DMA ///< asynchronous transfer (DMA)
};

// Status of the transmission method.
TxState State = TxState::Default;

char usartString[USART_STRING_SIZE]; //The string that will be sent via usart.
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
    commands_decoder_init();

    // Setting the default state.  
    if ( HAL_GPIO_ReadPin( USER_LED_GPIO_Port, USER_LED_Pin ) == GPIO_PIN_SET )
    {
        HAL_GPIO_WritePin( USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET );
    }
    ssd1306_Init();

   HAL_IWDG_Refresh(&hiwdg); 
  //FEE_Write(FEE_START_ADDRESS, (uint16_t*)PassWord_W, sizeof(char)*9);
  FEE_Read(FEE_START_ADDRESS, (uint16_t*)PassWord_R, sizeof(char)*9);
  sprintf(usartString, "Read= %s\r\n", PassWord_R);
  HAL_UART_Transmit( & UART, ( uint8_t * ) usartString, strlen(usartString), 50 );
  HAL_Delay(500);

   // uint8_t *buff = flash_read_test();
     
    //HAL_UART_Transmit(&UART, buff, 24, 50);
    //flash_write_test();
}


void printWine()
{
 if(logoSwith)
     {
       logoSwith = false;
       //ssd1306_WriteString("Got beer?", Font_11x18, White);
       ssd1306_SetCursor(2,0);
       ssd1306_WriteString("Would you like", Font_7x10, White);
       ssd1306_SetCursor(2,15);
       ssd1306_WriteString("some wine?", Font_7x10, White);
       ssd1306_SetCursor(0,24);
       ssd1306_WriteString("chat gpt say", Font_6x8, White);
     }
     else
     {
       //ssd1306_WriteString("Got beer ?", Font_11x18, White); 
       logoSwith = true;
       ssd1306_SetCursor(2,0);
       ssd1306_WriteString("Would you like", Font_7x10, White);
       ssd1306_SetCursor(2,15);
       ssd1306_WriteString("some wine ?", Font_7x10, White);
     }   
}

void voltageToString(uint32_t adc, char* str)
{
   float voltage = adc  * ADC_REFERENCE_VOLTAGE / ADC_MAX;
   sprintf(str,"%d.%02d ", (uint32_t)voltage, (uint16_t)((voltage - (uint32_t)voltage) * 100.));
}

/**
 * \brief   It is performed periodically in the body of the main loop.
 *
 */

void loop( void )
{
    HAL_Delay( 1000 );
     //HAL_GPIO_TogglePin( USER_LED_GPIO_Port, USER_LED_Pin);

    HAL_IWDG_Refresh(&hiwdg);

    //ssd1306_Fill(Black);
    //printWine();     
    //ssd1306_UpdateScreen();
    //printString("Привет", 3);
    ssd1306_SetCursor(0,0);
  
     //   ssd1306_WriteString("454", Font_7x10, White);
     char *test ="Привет";
     char test1[20];
    //sprintf(test1,"%02X", test[2]);
    //ssd1306_WriteString(test1, Font_7x10, White);
    //ssd1306_PrintChar(76);
    ssd1306_PrintString("Привет мир hello");
    ssd1306_UpdateScreen();
   return;

  if(!needSendDataToUSART()) return ;
  // We are waiting for the end of the packet transmission.
  if (UART.gState != HAL_UART_STATE_READY ) return;

  int strPos = 0;
  for(int i = 0; i < ADC_NUMBER_OF_CHANNELS; i++)
  {
    voltageToString(adcData[i], usartString + i * VOLTAGE_STR_LEN + 1);
    strPos+= VOLTAGE_STR_LEN;
  }

  HAL_IWDG_Refresh(&hiwdg);
  sprintf(usartString + strPos + 1, "u\n");
  HAL_UART_Transmit( & UART, ( uint8_t * ) usartString, sizeof( usartString ), 50 );
}

void enableBeeper(bool enabled)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};
   GPIO_InitStruct.Pin = GPIO_PIN_1;
   
   if(enabled)
   {
     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
   }
   else
   {
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
   }
   
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

int beep = 20;
bool alarm = false;
void HAL_SYSTICK_Callback( void )
{
    TimeTickMs++;

    if (TimeTickMs - oldTimeTickHSec > 1000)
    {
        oldTimeTickHSec = TimeTickMs;
        secondTickHandler();
        // Indication of the main cycle operation.
      //HAL_GPIO_TogglePin( USER_LED_GPIO_Port, USER_LED_Pin);
   
    }    
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{   
   if (htim->Instance == TIM2)
   {
    // HAL_GPIO_TogglePin( USER_LED_GPIO_Port, USER_LED_Pin);
    // Indication of the main cycle operation.
         if(beep > 30) beep = 10;

    beep++;
//HAL_GPIO_TogglePin( USER_LED_GPIO_Port, USER_LED_Pin);
      //__HAL_TIM_SET_AUTORELOAD(&htim3, beep*2);
     // __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_4, beep);

    //enableBeeper(alarm);
    alarm = !alarm;

  
   }
}

//When all data from usart is received, an interrupt will be triggered.
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  receive_byte();
}