#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define INTERVAL_FOR_SENDING_INFO 10 //The interval for sending messages via usart in seconds.

_BEGIN_STD_C
//Сalled from the outside every second.
void secondTickHandler(void);
bool needSendDataToUSART();

_END_STD_C