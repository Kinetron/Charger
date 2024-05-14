#include "soft_timers.h"

int secondCounter = 0;

//Сalled from the outside every second.
void secondTickHandler(void)
{
   secondCounter ++;
}

bool needSendDataToUSART()
{
    if(secondCounter == INTERVAL_FOR_SENDING_INFO)
    {
        secondCounter = 0;
        return true;
    }

    return false;
}