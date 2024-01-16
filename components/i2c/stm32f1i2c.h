#ifndef MAIN_STM32F1_H_
#define MAIN_STM32F1_H_
#include <time.h>
#include <stdbool.h>
#include "driver/i2c.h"

#include "stm32f1i2c.h"
#include "string.h"
#include <stddef.h>
#include "i2cdev.h"
#include "ds3231.h"
#define STM32F1_ADDR 0x12 //!< I2C address
#define DATA_STM32F1_ADDR 0x12
char data_stm32f1_full[500];

esp_err_t stm32f1_init(i2c_dev_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t stm32f1_get_data(i2c_dev_t *dev, char *data, size_t size);
void processing_string_Receive(char inputStrings[]);
#endif /* MAIN_STM32F1_H_ */
