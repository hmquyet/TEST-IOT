#ifndef MODBUS_MASTER__H
#define MODBUS_MASTER__H
//8N2 , 19200 , ID 126 , 0X03 
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
// #include "esp_check.h"
#include "nvs.h"
#include "cJSON.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "sdkconfig.h"
#include "math.h"

#include "esp_sntp.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "ds3231.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "soc/soc_caps.h"
#include <time.h>
#include <sys/time.h>
#include "esp_event_loop.h"
#include "esp_task_wdt.h"
#include "esp_err.h"

#include "modbus_params.h"  
#include "mbcontroller.h"
#include <endian.h>
#include <stdint.h>


#define MB_PORT_NUM     (CONFIG_MB_UART_PORT_NUM)   
#define MB_DEV_SPEED    (19200) 
#define MASTER_MAX_CIDS num_device_parameters
#define UPDATE_CIDS_TIMEOUT_MS          (150)
#define UPDATE_CIDS_TIMEOUT_TICS        (UPDATE_CIDS_TIMEOUT_MS / portTICK_RATE_MS)
// Timeout between polls
#define POLL_TIMEOUT_MS                 (150)
#define POLL_TIMEOUT_TICS               (POLL_TIMEOUT_MS / portTICK_RATE_MS)

// The macro to get offset for parameter in the appropriate structure
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))
#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) + 1))
#define COIL_OFFSET(field) ((uint16_t)(offsetof(coil_reg_params_t, field) + 1))
// Discrete offset macro
#define DISCR_OFFSET(field) ((uint16_t)(offsetof(discrete_reg_params_t, field) + 1))

enum {
    MB_DEVICE_ADDR1 = 126 // ADDRESS MODBUS 
};

enum {
    CID_UMIDADE = 0,
};

#define STR(fieldname) ((const char*)( fieldname ))
// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

float bigEndianToFloat(uint32_t bigEndianValue) ;
float littleEndianToFloat(uint32_t littleEndianValue) ;
/*
Chuyển little endian swap bit to big endian 
manual pac 3100 output rtu big endian 
input esp32 rs485 little swap endian 
*/
float transferendian(uint32_t data_read);



static void* master_get_param_data(const mb_parameter_descriptor_t* param_descriptor);

// User operation function to read slave values and check alarm
static void master_operation_func(void *arg);

// Modbus master initialization
static esp_err_t master_init(void);
#endif