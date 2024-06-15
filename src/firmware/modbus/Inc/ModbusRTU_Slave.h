/*
 * Modbus_Lib.h
 *
 *  Created on: Feb 10, 2022
 *      Author: Ümit Can Güveren
 */

#include "stdbool.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "stdbool.h"

_BEGIN_STD_C

#define SLAVEID  171
#define NUMBER_OF_REGISTER 10
#define NUMBER_OF_ANALOG_REGISTER 2


#define NUMBER_OF_COIL 30
#define BUFFERSIZE (NUMBER_OF_REGISTER * 2 + 5)

//For switch rx/tx mode. PB14
#define RS485_TX_EN_PIN GPIO_PIN_14
#define RS485_TX_EN_PORT GPIOB

enum
{
	ReadCoil = 0x01,
	ReadHoldingRegister = 0x03,
	ReadInputRegisters = 0x04,
	WriteSingleCoil = 0x05,
	WriteSingleRegister = 0x06,
	WriteMultipleCoils = 0x0F,
	WriteMultipleResisters = 0x10,
};


extern uint8_t uartRxData;
extern uint8_t DataCounter;
extern uint8_t RxInterruptFlag;
extern uint8_t uartTimeCounter;
extern uint8_t uartPacketComplatedFlag;
extern char ModbusRx[BUFFERSIZE];

/* Receiver functions */
void uartDataHandler(void);
void uartTimer(void);
void transmitDataMake(char *msg, uint8_t Lenght);


/* Modbus Functions */
void makePacket_01(char *msg, uint8_t Lenght);
void makePacket_03(char *msg, uint8_t Lenght);
void makePacket_04(char *msg, uint8_t Lenght);
void makePacket_05(char *msg, uint8_t Lenght);
void makePacket_06(char *msg, uint8_t Lenght);
void makePacket_15(char *msg, uint8_t Lenght);
void makePacket_16(char *msg, uint8_t Lenght);


void sendMessage(char *msg, uint8_t len);
uint8_t findByte(int16_t NumberOfCoil);
uint16_t MODBUS_CRC16(char *buf, uint8_t len );


_END_STD_C
