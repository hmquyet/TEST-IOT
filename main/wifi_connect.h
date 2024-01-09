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

#define CONFIG_WIFI_SSID "DESKTOP" //5k 1 tiếng
#define CONFIG_WIFI_PASSWORD "11111111" //12345678

// #define SSID CONFIG_WIFI_SSID
// #define PASSWORD CONFIG_WIFI_PASSWORD


TaskHandle_t taskHandle;
 int reconnect_time;
 bool boot_to_reconnect;

 void wifiInit(void);
 static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);