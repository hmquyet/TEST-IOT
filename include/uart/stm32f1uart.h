#ifndef LORASX1278__H
#define LORASX1278__H
#include "esp_err.h"
#include "hal/gpio_types.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/uart.h"


#define UART_NUM UART_NUM_2
#define TX_GPIO_NUM (GPIO_NUM_17)
#define RX_GPIO_NUM (GPIO_NUM_16)
#define PATTERN_CHR_NUM (3)
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

static QueueHandle_t uart2_queue = NULL;
static uart_event_t event;

void stm32f1_uart_config();

void processing_string_Receive_uart(char inputStrings[] ,char name[],char value[]);
void stm32f1_reciever_uart(char *data);

void stm32f1_tranmister_uart();
void process_string_Tranmister(char name[],char value[], char outputStrings[], size_t outputSize );

#endif