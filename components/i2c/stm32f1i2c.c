#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "stm32f1i2c.h"
#include "main.h"

#define CHECK_ARG(ARG) do { if (!ARG) return ESP_ERR_INVALID_ARG; } while (0)

esp_err_t stm32f1_init(i2c_dev_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);

    dev->port = port;
    dev->addr = STM32F1_ADDR;
    dev->sda_io_num = sda_gpio;
    dev->scl_io_num = scl_gpio;
    dev->clk_speed = I2C_FREQ_HZ;
    return i2c_master_init(port, sda_gpio, scl_gpio);
}
esp_err_t stm32f1_get_data(i2c_dev_t *dev, char *data, size_t size)
{
    CHECK_ARG(dev);

     esp_err_t res = i2c_dev_read_reg(dev,DATA_STM32F1_ADDR , data, size);

    return res;
}

void processing_string_Receive(char inputStrings[])
{

  
    typedef struct
    {
        char name[50];
        char value[50];
      
    } Node;
    Node nodes[5];
    int nodeCount = 0;
    char *nodePos = strstr(inputStrings, "!DATA");

    if ((inputStrings[0] == '!') && (inputStrings[strlen(inputStrings) - 1] == '#'))
    {
        for (int i = 0; i < strlen(inputStrings); i++)
        {

            if (nodePos != NULL)
            {

                  // int nodeIndex = inputStrings[5] - '0' - 1;
                int nodeIndex = 0;

                // Xử lý tên, giá trị và timestamp từ chuỗi ban đầu
                if (strstr(inputStrings, "name:") != NULL)
                {
                    char *indexStart = strstr(inputStrings, ":") + 5 + 1;
                    char *indexEnd = strstr(inputStrings, "#");

                    strncpy(nodes[nodeIndex].name, indexStart, indexEnd - indexStart);
                    nodes[nodeIndex].name[indexEnd - indexStart] = '\0';
                }
                else if (strstr(inputStrings, "value:") != NULL)
                {
                    char *indexStart = strstr(inputStrings, ":") + 6 + 1;
                    char *indexEnd = strstr(inputStrings, "#");

                    strncpy(nodes[nodeIndex].value, indexStart, indexEnd - indexStart);
                    nodes[nodeIndex].value[indexEnd - indexStart] = '\0';
                }

            }
        }
        ds3231_get_time(&rtc_i2c, &local_time);
        snprintf(data_stm32f1_full, sizeof(data_stm32f1_full), "[{\"name\":\"%s\",\"value\":%s,\"timestamp\":%04d-%02d-%02dT%02d:%02d:%02d}]\n", nodes[0].name, nodes[0].value,local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec);
        printf(data_stm32f1_full);
      
    }


}