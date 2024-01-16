#ifndef WIFI_MQTT__H
#define WIFI_MQTT__H
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

#include "stm32f1uart.h"

#define MAX_RETRY 10
#define EXAMPLE_ESP_WIFI_SSID "HQm"      // PDA_CHA_NhaDuoi  DESKTOPAI
#define EXAMPLE_ESP_WIFI_PASS "12345679" // Tiaportal  10101010

int WIFI_CONNECTED ;


TaskHandle_t taskHandle;
 int reconnect_time;
 bool boot_to_reconnect;

 void wifiInit(void);
esp_err_t wifi_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data);

                             #include "stm32f1uart.h"

//static esp_mqtt_client_handle_t client ;

char topic_buffer[]; 
char data_buffer[];  
// void mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
// void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_app_start(void);
//void mqtt_mess_task(void *arg);
void mqtt_data_processing(char *mqttJson);
#endif