#include "main.h"
#include "usart.h"

#define ADC_CALIBRATION_COMMAND "adc_calibration"

_BEGIN_STD_C

void commands_decoder_init(void);
void receive_byte(void);
void commands_decoder_add_to_buff(void);
void clear_buffer(void);

_END_STD_C