#ifndef SD_CARD__H
#define SD_CARD__H
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



#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 15 // 14 cho board thuong mai, 15 cho board tu lam
#define SPI_DMA_CHAN 1
bool error_sd_card;

#define MOUNT_POINT "/sdcard"


void write_to_sd(char content[], char file_path[]);
void sdcard_mount();
esp_err_t unmount_card(const char *base_path, sdmmc_card_t *card);
#endif