cmake_minimum_required(VERSION 3.19)

target_include_directories(${PROJECT_NAME} PRIVATE Inc )

target_sources(${PROJECT_NAME} PRIVATE
    Src/syscalls.c
    Src/sysmem.c
    Src/stm32f1xx_hal_msp.c
    Src/stm32f1xx_it.c
    Src/iwdg.c
    Src/gpio.c
    Src/tim.c
    Src/i2c.c
    Src/usart.c
    Src/dma.c
    Src/adc.c
    Src/main.c
)
