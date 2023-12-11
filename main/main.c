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
#include "button.c"
#include "esp_sntp.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "ds3231.h"
#include "file_server.c"
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

#define TAG "MQTT_DAQ"
#define STORAGE_NAMESPACE "esp32_storage" // save variable to nvs storage

#define TIMER_DIVIDER (16)                           //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

// Define pin to read Cycle period, Open time, Machine's status
#define GPIO_INPUT_FINE_GRINDER 36 // 36
#define GPIO_INPUT_CYCLE_2 39 // 39

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 14 // 14 cho board thuong mai, 15 cho board tu lam

#define SPI_DMA_CHAN 1
#define MOUNT_POINT "/sdcard"

#define CONFIG_WIFI_SSID "Fablab 1"   // 5k 1 tiếng
#define CONFIG_WIFI_PASSWORD "Fablab123" // 12345678

#define SSID CONFIG_WIFI_SSID
#define PASSWORD CONFIG_WIFI_PASSWORD

typedef struct
{
  int timer_group;
  int timer_idx;
  int alarm_interval;
  bool auto_reload;
} timer_info_t;

typedef struct
{
  char *timestamp;
  char *moldId;
  char *productId;
  double cycle_cfg;
  bool is_configured;
} cfg_info_t;

typedef struct
{
  int counter_shot;
  char *timestamp;
  double cycle_time;
  double open_time;
  bool mode_working;
  int status;
} cycle_info_t;

typedef struct
{
  char *timestamp;
  int command;
} da_message_t;

enum feedback
{
  SychTime,
  SDcardfail,
  RTCfail
};
enum da_mess
{
  ChangeMold,
  ChangeMoldDone,
  Reboot
};
enum machine
{
  PowerOff,
  Running,
  PowerOn,
  Disconnect,
  Connected,
  Idle,

};
static esp_mqtt_client_handle_t client = NULL;
static xQueueHandle button_events;
static xQueueHandle mqtt_mess_events;
TaskHandle_t taskHandle;
TaskHandle_t initiate_taskHandle;
TimerHandle_t soft_timer_handle_1;
TimerHandle_t soft_timer_handle_2;
TimerHandle_t soft_timer_handle_3;
TimerHandle_t soft_timer_handle_4;
TimerHandle_t soft_timer_handle_5;
TimerHandle_t soft_timer_handle_6;
TimerHandle_t soft_timer_handle_7;
i2c_dev_t rtc_i2c;
cycle_info_t cycle_info;
da_message_t da_message;
cfg_info_t cfg_info;

struct tm local_time;
struct tm inject_time;
struct tm alarm_time;
struct tm idle_time;

const uint32_t WIFI_CONNEECTED = BIT1;
const uint32_t MQTT_CONNECTED = BIT2;
const uint32_t MQTT_PUBLISHED = BIT3;
const uint32_t SYSTEM_READY = BIT4;
const uint32_t INITIATE_SETUP = BIT5;
const uint32_t SYNC_TIME_DONE = BIT6;
const uint32_t SYNC_TIME_FAIL = BIT7;

// const char* CYCLE_TIME_TOPIC = "IMM/I2/Metric/CycleMessage";
const char *MACHINE_STATUS_TOPIC = "IMM/I2/Metric/MachineStatus";
const char *INJECTION_CYCLE_TOPIC = "IMM/I2/Metric/InjectionCycle";
const char *INJECTION_TIME_TOPIC = "IMM/I2/Metric/InjectionTime";


const char *FINE_GRINDER_STATUS_TOPIC = "FALAB/MACHANICAL MACHINES/FineGrinder/Metric/FineGrinderStatus";
const char *FINE_GRINDER_LAST_TIME_TOPIC = "FALAB/MACHANICAL MACHINES/FineGrinder/Metric/LastFineGrinderTime";


static char MACHINE_STATUS_FILE[50] = "/sdcard/status.csv";
static char MACHINE_RUNNING_TIME_FILE[50] = "/sdcard/running.csv";
static char MACHINE_GRINDING_TIME_FILE[50] = "/sdcard/grinding.csv";

const char *MACHINE_LWT_TOPIC = "IMM/I2/Metric/LWT";
const char *DA_MESSAGE_TOPIC = "IMM/I2/Metric/DAMess";
const char *CONFIGURATION_TOPIC = "IMM/I2/Metric/ConfigMess";
const char *SYNC_TIME_TOPIC = "IMM/I2/Metric/SyncTime";
const char *FEEDBACK_TOPIC = "IMM/I2/Metric/Feedback";
const char *MOLD_ID_DEFAULT = "NotConfigured";
const char *RECONNECT_BROKER_TIMER = "RECONNECT_BROKER_TIMER";
const char *OPEN_CHECK_TIMER = "OPEN_CHECK_TIMER";
const char *INITIATE_TASK_TIMER = "INITIATE_TASK_TIMER";
const char *SHIFT_REBOOT_TIMER = "SHIFT_REBOOT_TIMER";
const char *STATUS_TIMER = "STATUS_TIMER";
const char *RECONNECT_TIMER = "RECONNECT_TIMER";
const char *BOOT_CONNECT_TIMER = "BOOT_CONNECT_TIMER";

static int cycle_id;
static int reconnect_time;
static int error_time_local;
static int reboot_timer_flag;
static int64_t remain_time;
static bool error_sd_card;
static bool error_rtc;
static bool boot_to_reconnect;
static bool idle_trigger;
static cJSON *my_json;
static char CURRENT_CYCLE_FILE[50] = "/sdcard/c1090422.csv";
static char CURRENT_STATUS_FILE[50] = "/sdcard/s1090422.csv";

/*----------------------------------------------------------------------*/
esp_err_t start_file_server(const char *base_path);
/*----------------------------------------------------------------------*/
/**
 * @brief Delete the task that call this fuction
 *
 */
static void delete_task()
{
  vTaskDelete(NULL);
}
/*----------------------------------------------------------------------*/
/**
 * @brief Write content to file in SD card in append+ mode
 *
 * @param content Content to write
 * @param file_path File path to write
 */
void write_to_sd(char content[], char file_path[])
{
  FILE *f = fopen(file_path, "a+");
  if (f == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for writing --> Restart ESP");
    esp_restart();
    return;
  }
  int i = 0;
  while (content[i] != NULL)
    i++;

  char buff[i + 1];
  for (int j = 0; j < i + 1; j++)
  {
    buff[j] = content[j];
  }
  fprintf(f, buff);
  fprintf(f, "\n");
  fclose(f);
  ESP_LOGI(TAG, "File written");
}
/*----------------------------------------------------------------------*/
/**
 * @brief Mount SD card using SPI bus
 *
 */
static void sdcard_mount()
{
  /*sd_card part code*/
  esp_vfs_fat_sdmmc_mount_config_t mount_config =
      {
          .format_if_mount_failed = true,
          .max_files = 5,
          .allocation_unit_size = 16 * 1024};
  sdmmc_card_t *card;
  const char mount_point[] = MOUNT_POINT;
  ESP_LOGI(TAG, "Initializing SD card");

  ESP_LOGI(TAG, "Using SPI peripheral");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  spi_bus_config_t bus_cfg = {
      .mosi_io_num = PIN_NUM_MOSI,
      .miso_io_num = PIN_NUM_MISO,
      .sclk_io_num = PIN_NUM_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
  };

  esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize bus.");
  }

  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = PIN_NUM_CS;
  slot_config.host_id = host.slot;
  esp_err_t err = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (err != ESP_OK)
  {
    error_sd_card = true;
    char mess_fb[200];
    sprintf(mess_fb, "{%cMess%c:%d}", 34, 34, SDcardfail);

    if (esp_mqtt_client_publish(client, FEEDBACK_TOPIC, mess_fb, 0, 1, 0) == -1)
    {
      esp_mqtt_client_enqueue(client, FEEDBACK_TOPIC, mess_fb, 0, 1, 0, 1);
    }
    if (err == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize the card. "
                    "Make sure SD card lines have pull-up resistors in place.");
    }
  }
  else if (err == ESP_OK)
  {
    sdmmc_card_print_info(stdout, card);
    ESP_ERROR_CHECK(start_file_server("/sdcard"));
  }
}
/*----------------------------------------------------------------------*/
static esp_err_t unmount_card(const char *base_path, sdmmc_card_t *card)
{
  esp_err_t err = esp_vfs_fat_sdcard_unmount(base_path, card);

  ESP_ERROR_CHECK(err);

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  err = spi_bus_free(host.slot);

  ESP_ERROR_CHECK(err);

  return err;
}
/*----------------------------------------------------------------------*/
/**
 * @brief Luu du lieu vao bo nho nvs cua ESP32
 *
 * @param value2write con tro toi gia tri duoc ghi
 * @param keyvalue ten bien can luu trong nvs
 */
esp_err_t save_value_nvs(int *value2write, char *keyvalue)
{
  nvs_handle_t my_handle;
  esp_err_t err;

  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);

  switch (err)
  {
  case ESP_OK:

    err = nvs_set_i32(my_handle, keyvalue, *value2write);
    err = nvs_commit(my_handle);
    nvs_close(my_handle);
    return err;
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGI(TAG, "The value is not initialized yet!\n");

    err = nvs_set_i32(my_handle, keyvalue, *value2write);

    err = nvs_commit(my_handle);
    nvs_close(my_handle);
    return err;
    break;
  default:
    nvs_close(my_handle);

    return err;
  }
}
/*----------------------------------------------------------------------*/
/**
 * @brief Doc du lieu tu bo nho nvs cua ESP32 va luu vao bien value2read
 *
 * @param value2read con tro toi gia tri can duoc load vao
 * @param keyvalue ten bien trong nvs
 */
esp_err_t load_value_nvs(int *value2read, char *keyvalue)
{
  nvs_handle_t my_handle;
  esp_err_t err;

  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
    return err;
  err = nvs_get_i32(my_handle, keyvalue, value2read);
  switch (err)
  {
  case ESP_OK:
    nvs_close(my_handle);
    return ESP_OK;
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGI(TAG, "The value is not initialized yet!\n");
    err = nvs_set_i32(my_handle, keyvalue, *value2read);
    err = nvs_commit(my_handle);
    nvs_close(my_handle);
    return err;
    break;
  default:
    nvs_close(my_handle);
    return err;
  }
}
/*----------------------------------------------------------------------*/
void set_clock(struct tm *timeinfo)
{
  struct tm time2set = {
      .tm_year = timeinfo->tm_year,
      .tm_mon = timeinfo->tm_mon,
      .tm_mday = timeinfo->tm_mday,
      .tm_hour = timeinfo->tm_hour,
      .tm_min = timeinfo->tm_min,
      .tm_sec = timeinfo->tm_sec};
  if (ds3231_set_time(&rtc_i2c, &time2set) != ESP_OK)
  {
    ESP_LOGE(pcTaskGetTaskName(0), "Could not set time.");
    while (1)
    {
      vTaskDelay(1);
    }
  }
  ESP_LOGI(pcTaskGetTaskName(0), "Set date time done");

  delete_task();
}
/*----------------------------------------------------------------------*/
static int calculate_diff_time(struct tm start_time, struct tm end_time)
{
  int diff_time;
  int diff_hour;
  int diff_min;
  int diff_sec;
  int e_sec = 0;
  int e_min = 0;

  if (end_time.tm_sec < start_time.tm_sec)
  {
    diff_sec = end_time.tm_sec + 60 - start_time.tm_sec;
    e_sec = 1;
  }
  else
  {
    diff_sec = end_time.tm_sec - start_time.tm_sec;
  }

  if (end_time.tm_min < start_time.tm_min)
  {
    diff_min = end_time.tm_min - e_sec + 60 - start_time.tm_min;
    e_min = 1;
  }
  else
  {
    diff_min = end_time.tm_min - e_sec - start_time.tm_min;
  }

  if (end_time.tm_hour < start_time.tm_hour)
  {
    diff_hour = end_time.tm_hour - e_min + 24 - start_time.tm_hour;
  }
  else
  {
    diff_hour = end_time.tm_hour - e_min - start_time.tm_hour;
  }

  diff_time = (diff_hour * 3600 + diff_min * 60 + diff_sec) * 1000;

  return diff_time;
}
/*----------------------------------------------------------------------*/
void get_tomorrow(struct tm *tomorrow, struct tm *today)
{
  int year = today->tm_year;
  int mon = today->tm_mon;
  int mday = today->tm_mday;
  int leap_year = year % 4;

  tomorrow->tm_year = today->tm_year;
  tomorrow->tm_mon = today->tm_mon;
  tomorrow->tm_mday = today->tm_mday + 1;

  if ((mday == 31) && (mon == 1))
  {
    tomorrow->tm_mon = 2;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 28) && (mon == 2))
  {
    if (leap_year != 0)
    {
      tomorrow->tm_mon = 3;
      tomorrow->tm_mday = 1;
    }
    else
    {
      tomorrow->tm_mon = 2;
      tomorrow->tm_mday = 29;
    }
    return;
  }
  if ((mday == 29) && (mon == 2))
  {
    tomorrow->tm_mon = 3;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 3))
  {
    tomorrow->tm_mon = 4;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 30) && (mon == 4))
  {
    tomorrow->tm_mon = 5;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 5))
  {
    tomorrow->tm_mon = 6;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 30) && (mon == 6))
  {
    tomorrow->tm_mon = 7;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 7))
  {
    tomorrow->tm_mon = 8;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 8))
  {
    tomorrow->tm_mon = 9;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 30) && (mon == 9))
  {
    tomorrow->tm_mon = 10;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 10))
  {
    tomorrow->tm_mon = 11;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 30) && (mon == 11))
  {
    tomorrow->tm_mon = 12;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 12))
  {
    tomorrow->tm_mon = 1;
    tomorrow->tm_mday = 1;
    tomorrow->tm_year = tomorrow->tm_year + 1;
    return;
  }
  return;
}
/*-------------------------------------------------------------------*/
void get_previous_day(struct tm *previous, struct tm *today)
{
  int year = today->tm_year;
  int mon = today->tm_mon;
  int mday = today->tm_mday;
  int leap_year = year % 4;

  previous->tm_year = today->tm_year;
  previous->tm_mon = today->tm_mon;
  previous->tm_mday = today->tm_mday - 1;

  if ((mday == 1) && (mon == 1))
  {
    previous->tm_mon = 12;
    previous->tm_mday = 31;
    previous->tm_year = previous->tm_year - 1;
    return;
  }
  if ((mday == 1) && (mon == 2))
  {
    previous->tm_mon = 1;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 3))
  {
    if (leap_year != 0)
    {
      previous->tm_mon = 2;
      previous->tm_mday = 28;
    }
    else
    {
      previous->tm_mon = 2;
      previous->tm_mday = 29;
    }
    return;
  }
  if ((mday == 1) && (mon == 4))
  {
    previous->tm_mon = 3;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 5))
  {
    previous->tm_mon = 4;
    previous->tm_mday = 30;
    return;
  }
  if ((mday == 1) && (mon == 6))
  {
    previous->tm_mon = 5;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 7))
  {
    previous->tm_mon = 6;
    previous->tm_mday = 30;
    return;
  }
  if ((mday == 1) && (mon == 8))
  {
    previous->tm_mon = 7;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 9))
  {
    previous->tm_mon = 8;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 10))
  {
    previous->tm_mon = 9;
    previous->tm_mday = 30;
    return;
  }
  if ((mday == 1) && (mon == 11))
  {
    previous->tm_mon = 31;
    previous->tm_mday = 10;
    return;
  }
  if ((mday == 1) && (mon == 12))
  {
    previous->tm_mon = 30;
    previous->tm_mday = 11;
    return;
  }

  return;
}
/*----------------------------------------------------------------------*/
void restart_esp()
{
  cycle_id = 0;
  save_value_nvs(&cycle_id, "cycle_id");
  esp_restart();
}
/*----------------------------------------------------------------------*/
static void gpio_task(void *arg)
{
  double injection_time = 0;
  double injection_time_1 = 0;
  double last_fine_grinder_time = 0;
  double injection_cycle = 0;
  double temp_open_time = 0;
  int last_open_id = 0;

  char message_text[1000];
  char message_mqtt[1000];

  button_event_t ev;
  bool open_door_flag = false;
  // TWDT
  esp_task_wdt_reset();

  sdcard_mount();

  cycle_id = 0;
  int hardwaretimer;

  while (1)
  {
    esp_task_wdt_add(NULL);
    if (xQueueReceive(button_events, &ev, 1000 / portTICK_PERIOD_MS))
    {
      // ESP_LOGI("TAG","Button event");
      // if ((ev.pin == GPIO_INPUT_CYCLE_1) && (ev.event == BUTTON_DOWN))
      // {
      //   ESP_LOGI("TAG", "I2");
      //   ESP_LOGI("GPIO", "Event BUTTON_DOWN %s", "GPIO_INPUT_CYCLE_1");

      //   xTimerStop(soft_timer_handle_5, 10); // Timer check idle status
      //   xTimerStart(soft_timer_handle_5, 10);

      //   timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL); // Set cho timer ve gia tri 0
      //   timer_start(TIMER_GROUP_0, TIMER_0);

      //   // if (last_running_time > 1.0)
      //   // {

      //   ds3231_get_time(&rtc_i2c, &local_time);
      //   sprintf(message_mqtt, "[{%cname%c: %cmachineStatus%c,%cvalue%c: %d,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}]",
      //           34, 34, 34, 34, 34, 34, Running, 34, 34, 34, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, 34);
      //   esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 1);

      //   // sprintf(message_text, "%04d-%02d-%02dT%02d:%02d:%02d,%d",
      //   //         local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, Running);
      //   // write_to_sd(message_text, MACHINE_STATUS_FILE);
      //   // printf(message_text);
      //   // TWDT
      //   esp_task_wdt_delete(NULL);
      //   // TWDT
      //   esp_task_wdt_add(NULL);
      //   // TWDT
      //   esp_task_wdt_reset();
      //   // }
      // }

      // else if ((ev.pin == GPIO_INPUT_CYCLE_1) && (ev.event == BUTTON_UP) && (open_door_flag == false))
      // {
      //   xTimerStop(soft_timer_handle_5, 10); // Timer check idle status
      //   xTimerStart(soft_timer_handle_5, 10);

      //   timer_pause(TIMER_GROUP_0, TIMER_0); // Dung timer dem thoi gian chu ky
      //   timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_0, &injection_time);

      //   ESP_LOGI("GPIO", "Event BUTTON_UP %s", "GPIO_INPUT_CYCLE_1");

      //   ds3231_get_time(&rtc_i2c, &local_time);
      //   sprintf(message_mqtt, "[{%cname%c: %cmachineStatus%c,%cvalue%c: %d,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}]",
      //           34, 34, 34, 34, 34, 34, Running, 34, 34, 34, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, 34);
      //   esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 1);

      //   // sprintf(message_text, "%04d-%02d-%02dT%02d:%02d:%02d,%d",
      //   //         local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, Idle);
      //   // write_to_sd(message_text, MACHINE_STATUS_FILE);
      //   //  printf(message_text);

      //   if (injection_time > 40)
      //   {
      //     ds3231_get_time(&rtc_i2c, &inject_time);
      //     sprintf(message_mqtt, "[{%cname%c: %cinjectionTime%c,%cvalue%c: %f,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}]",
      //             34, 34, 34, 34, 34, 34, injection_time, 34, 34, 34, inject_time.tm_year, inject_time.tm_mon, inject_time.tm_mday, inject_time.tm_hour, inject_time.tm_min, inject_time.tm_sec, 34);
      //     esp_mqtt_client_publish(client, INJECTION_TIME_TOPIC, message_mqtt, 0, 1, 1);
      //   }
      //   // sprintf(message_text, "%04d-%02d-%02dT%02d:%02d:%02d,%f",
      //   //         inject_time.tm_year, inject_time.tm_mon, inject_time.tm_mday, inject_time.tm_hour, inject_time.tm_min, inject_time.tm_sec, last_running_time);

      //   // write_to_sd(message_text, MACHINE_RUNNING_TIME_FILE);
      //   printf(message_text);
      //   // TWDT
      //   esp_task_wdt_reset();
      // }
      // else if ((ev.pin == GPIO_INPUT_CYCLE_2) && (ev.event == BUTTON_DOWN))
      // {
      //   ESP_LOGI("TAG", "I2");
      //   ESP_LOGI("GPIO", "Event BUTTON_DOWN %s", "GPIO_INPUT_CYCLE_2");

      //   xTimerStop(soft_timer_handle_5, 10); // Timer check idle status
      //   xTimerStart(soft_timer_handle_5, 10);

      //   timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0x00000000ULL); // Set cho timer ve gia tri 0
      //   timer_start(TIMER_GROUP_0, TIMER_1);

      //   // if (last_running_time > 1.0)
      //   // {

      //   ds3231_get_time(&rtc_i2c, &local_time);
      //   sprintf(message_mqtt, "[{%cname%c: %cmachineStatus%c,%cvalue%c: %d,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}]",
      //           34, 34, 34, 34, 34, 34, Running, 34, 34, 34, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, 34);

      //   esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 1);

      //   // sprintf(message_text, "%04d-%02d-%02dT%02d:%02d:%02d,%d",
      //   //         local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, Grinding);
      //   // write_to_sd(message_text, MACHINE_STATUS_FILE);
      //   //printf(message_text);

      //   // TWDT
      //   esp_task_wdt_delete(NULL);

      //   // TWDT
      //   esp_task_wdt_add(NULL);
      //   // TWDT
      //   esp_task_wdt_reset();
      //   // }
      // }

      // else if ((ev.pin == GPIO_INPUT_CYCLE_2) && (ev.event == BUTTON_UP))
      // {
      //   xTimerStop(soft_timer_handle_5, 10); // Timer check idle status
      //   xTimerStart(soft_timer_handle_5, 10);

      //   timer_pause(TIMER_GROUP_0, TIMER_1); // Dung timer dem thoi gian chu ky
      //   timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_1, &injection_time_1);
      //   injection_cycle = injection_time + injection_time_1 ;
      //   ESP_LOGI("GPIO", "Event BUTTON_UP %s", "GPIO_INPUT_CYCLE_2");

      //   ds3231_get_time(&rtc_i2c, &local_time);
      //   sprintf(message_mqtt, "[{%cname%c: %cmachineStatus%c,%cvalue%c: %d,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}]",
      //           34, 34, 34, 34, 34, 34, Running, 34, 34, 34, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, 34);
      //   esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 1);

      //   // sprintf(message_text, "%04d-%02d-%02dT%02d:%02d:%02d,%d",
      //   //         local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, Idle);
      //   // write_to_sd(message_text, MACHINE_STATUS_FILE);
      //   printf(message_text);

      //   if (injection_cycle > 40)
      //   {
      //     ds3231_get_time(&rtc_i2c, &inject_time);
      //     sprintf(message_mqtt, "[{%cname%c: %cinjectionCycle%c,%cvalue%c: %f,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}]",
      //             34, 34, 34, 34, 34, 34, injection_cycle, 34, 34, 34, inject_time.tm_year, inject_time.tm_mon, inject_time.tm_mday, inject_time.tm_hour, inject_time.tm_min, inject_time.tm_sec, 34);

      //     esp_mqtt_client_publish(client, INJECTION_CYCLE_TOPIC, message_mqtt, 0, 1, 1);
      //   }

      //   // sprintf(message_text, "%04d-%02d-%02dT%02d:%02d:%02d,%f",
      //   //         inject_time.tm_year, inject_time.tm_mon, inject_time.tm_mday, inject_time.tm_hour, inject_time.tm_min, inject_time.tm_sec, last_grinding_time);
      //   // write_to_sd(message_text, MACHINE_GRINDING_TIME_FILE);
      //   printf(message_text);
      //   // TWDT
      //   esp_task_wdt_reset();
      // }

      if ((ev.pin == GPIO_INPUT_FINE_GRINDER) && (ev.event == BUTTON_DOWN))
      {
        ESP_LOGI("TAG", "welding");
        ESP_LOGI("GPIO", "Event BUTTON_DOWN %s", "GPIO_INPUT_FINE_GRINDER");

        xTimerStop(soft_timer_handle_5, 10); // Timer check idle status
        xTimerStart(soft_timer_handle_5, 10);

           timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0x00000000ULL); // Set cho timer ve gia tri 0
        timer_start(TIMER_GROUP_0, TIMER_1);

        ds3231_get_time(&rtc_i2c, &local_time);
        sprintf(message_mqtt, "{%cname%c: %cFineGrinderStatus%c,%cvalue%c: %d,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}",
                34, 34, 34, 34, 34, 34, Running, 34, 34, 34, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, 34);
        esp_mqtt_client_publish(client, FINE_GRINDER_STATUS_TOPIC , message_mqtt, 0, 1, 1);

     printf(message_text);
        // TWDT
        esp_task_wdt_delete(NULL);

        // TWDT
        esp_task_wdt_add(NULL);
        // TWDT
        esp_task_wdt_reset();
        // }
      }

      else if ((ev.pin == GPIO_INPUT_FINE_GRINDER) && (ev.event == BUTTON_UP))
      {
        xTimerStop(soft_timer_handle_5, 10); // Timer check idle status
        xTimerStart(soft_timer_handle_5, 10);

          timer_pause(TIMER_GROUP_0, TIMER_1); // Dung timer dem thoi gian chu ky
        timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_1, &last_fine_grinder_time);

        ESP_LOGI("TAG", "welding");
        ESP_LOGI("GPIO", "Event BUTTON_UP %s", "GPIO_INPUT_FINE_GRINDER");
        // if (last_running_time > 1.0)
        // {
        ds3231_get_time(&rtc_i2c, &local_time);
        sprintf(message_mqtt, "{%cname%c: %cFineGrinderStatus%c,%cvalue%c: %d,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}",
                34, 34, 34, 34, 34, 34, Idle, 34, 34, 34, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, 34);
        esp_mqtt_client_publish(client, FINE_GRINDER_STATUS_TOPIC , message_mqtt, 0, 1, 1);
 printf(message_text);
         ds3231_get_time(&rtc_i2c, &inject_time);
        sprintf(message_mqtt, "{%cname%c: %cLastFineGrinderTime%c,%cvalue%c: %f,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}",
                34, 34, 34, 34, 34, 34, last_fine_grinder_time  , 34, 34, 34, inject_time.tm_year, inject_time.tm_mon, inject_time.tm_mday, inject_time.tm_hour, inject_time.tm_min, inject_time.tm_sec, 34);
        esp_mqtt_client_publish(client, FINE_GRINDER_LAST_TIME_TOPIC  , message_mqtt, 0, 1, 1);
 printf(message_text);
        // TWDT
        esp_task_wdt_delete(NULL);

        // TWDT
        esp_task_wdt_add(NULL);
        // TWDT
        esp_task_wdt_reset();
        // }
      }

      else
      {
        esp_task_wdt_reset();
      }
    }
    else
    {
      // ESP_LOGI("TAG","Nothing happen");
      // TWDT
      esp_task_wdt_reset();
    }
  }
}

/*----------------------------------------------------------------------*/
static void initiate_task(void *arg);
/*----------------------------------------------------------------------*/
static void vSoftTimerCallback(TimerHandle_t xTimer)
{ // esp_task_wdt_add(NULL);
  if (pcTimerGetName(xTimer) == RECONNECT_BROKER_TIMER)
  {
    xTimerStop(soft_timer_handle_1, 10);
    esp_mqtt_client_reconnect(client);
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
  }
  else if (pcTimerGetName(xTimer) == OPEN_CHECK_TIMER)
  {
    // esp_mqtt_client_publish(client,OPEN_TIME_ERROR_TOPIC,"Exceed Open Time",0,1,0);
  }
  else if (pcTimerGetName(xTimer) == INITIATE_TASK_TIMER)
  {
    xTimerStop(soft_timer_handle_3, 10);
    xTaskCreate(initiate_task, "Alarm task", 2048 * 2, NULL, 4, initiate_taskHandle);
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
  }
  else if (pcTimerGetName(xTimer) == SHIFT_REBOOT_TIMER)
  {
    if (reboot_timer_flag == 1)
    {
      reboot_timer_flag++;
      xTimerStop(soft_timer_handle_2, 10);
      xTimerChangePeriod(soft_timer_handle_2, pdMS_TO_TICKS(remain_time), 10);
      xTimerStart(soft_timer_handle_2, 10);
      ESP_LOGI(TAG, "Do day r nha %lld", remain_time);
      // TWDT
      // esp_task_wdt_reset();
    }
    else if (reboot_timer_flag == 2)
    {
      xTimerStop(soft_timer_handle_2, 10);
      ESP_LOGI("Restart", "SHIFT_REBOOT_TIMER");
      restart_esp();
      // TWDT
      esp_task_wdt_reset();
      // esp_task_wdt_delete(NULL);
    }
  }
  else if (pcTimerGetName(xTimer) == STATUS_TIMER)
  {
    idle_trigger = true;
    char message_text[500];
    char message_mqtt[500];

    if (error_rtc == false)
      ds3231_get_time(&rtc_i2c, &idle_time);
    xTimerStop(soft_timer_handle_5, 10);
    sprintf(message_mqtt, "[{%cname%c: %cmachineStatus%c,%cvalue%c: %d,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}]", 34, 34, 34, 34, 34, 34, Idle, 34, 34, 34, idle_time.tm_year, idle_time.tm_mon, idle_time.tm_mday, idle_time.tm_hour,
            idle_time.tm_min, idle_time.tm_sec, 34);
    if (esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 0) == -1)
    {
      esp_mqtt_client_enqueue(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 0, 1);
    }

    sprintf(message_text, "%04d-%02d-%02dT%02d:%02d:%02d,%d", idle_time.tm_year, idle_time.tm_mon, idle_time.tm_mday, idle_time.tm_hour,
            idle_time.tm_min, idle_time.tm_sec, Idle);
    write_to_sd(message_text, &CURRENT_STATUS_FILE);

    xTimerStart(soft_timer_handle_5, 10);
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
  }
  else if (pcTimerGetName(xTimer) == RECONNECT_TIMER)
  {
    xTimerStop(soft_timer_handle_6, 10);
    esp_wifi_connect();
    reconnect_time = 0;
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
  }
  else if (pcTimerGetName(xTimer) == BOOT_CONNECT_TIMER)
  {
    xTimerStop(soft_timer_handle_7, 10);
    ESP_LOGI("Restart", "BOOT_CONNECT_TIMER");
    esp_restart();
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
  }
}
/*----------------------------------------------------------------------*/
static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
  BaseType_t high_task_awoken = pdFALSE;
  return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}
/*----------------------------------------------------------------------*/
/**
 * @brief Initialize selected timer of timer group
 *
 * @param group Timer Group number, index from 0
 * @param timer timer ID, index from 0
 * @param auto_reload whether auto-reload on alarm event
 * @param timer_interval_sec interval of alarm
 */
static void timer_init_isr(int group, int timer, bool auto_reload, int timer_interval_sec)
{
  /* Select and initialize basic parameters of the timer */
  timer_config_t config = {
      .divider = TIMER_DIVIDER,
      .counter_dir = TIMER_COUNT_UP,
      .counter_en = TIMER_PAUSE,
      .alarm_en = TIMER_ALARM_DIS,
      .auto_reload = auto_reload,
  }; // default clock source is APB
  timer_init(group, timer, &config);

  /* Timer's counter will initially start from value below.
     Also, if auto_reload is set, this value will be automatically reload on alarm */
  timer_set_counter_value(group, timer, 0);

  /* Configure the alarm value and the interrupt on alarm. */
  timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE);
  timer_enable_intr(group, timer);

  timer_info_t *timer_info = calloc(1, sizeof(timer_info_t));
  timer_info->timer_group = group;
  timer_info->timer_idx = timer;
  timer_info->auto_reload = auto_reload;
  timer_info->alarm_interval = timer_interval_sec;
  timer_isr_callback_add(group, timer, timer_group_isr_callback, timer_info, 0);
  timer_start(group, timer);
}
/*----------------------------------------------------------------------*/
void mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{ // TWDT
  // esp_task_wdt_add(NULL);

  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    xTaskNotify(taskHandle, MQTT_CONNECTED, eSetValueWithOverwrite);
    xTimerStop(soft_timer_handle_1, 10);
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    xTimerStart(soft_timer_handle_1, 10);
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
    break;
  case MQTT_EVENT_PUBLISHED:
    // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    // xTaskNotify(taskHandle, MQTT_PUBLISHED, eSetValueWithOverwrite);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    xQueueSend(mqtt_mess_events, &event, portMAX_DELAY);
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
    break;
  default:
    // TWDT
    esp_task_wdt_reset();
    // esp_task_wdt_delete(NULL);
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
}
/*----------------------------------------------------------------------*/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  mqtt_event_handler_cb(event_data);
}
/*----------------------------------------------------------------------*/
static void mqtt_mess_task(void *arg)
{
  esp_mqtt_event_handle_t mqtt_event;
  while (1)
  {
    char message_text[1000];
    if (xQueueReceive(mqtt_mess_events, &mqtt_event, 1000 / portTICK_PERIOD_MS))
    {
      if (mqtt_event->topic_len == 24) // IMM/I2/ConfigMess/Metric
      {
        my_json = cJSON_Parse(mqtt_event->data);
        cfg_info.timestamp = cJSON_GetObjectItem(my_json, "Timestamp")->valuestring;
        cfg_info.moldId = cJSON_GetObjectItem(my_json, "MoldId")->valuestring;
        cfg_info.productId = cJSON_GetObjectItem(my_json, "ProductId")->valuestring;
        cfg_info.cycle_cfg = cJSON_GetObjectItem(my_json, "CycleTime")->valuedouble;
        cfg_info.is_configured = true;
        ESP_LOGI(TAG, "Timestamp:%s, moldId:%s, productId:%s, Cycle:%f", cfg_info.timestamp, cfg_info.moldId, cfg_info.productId, cfg_info.cycle_cfg);
        // TWDT
        esp_task_wdt_reset();
      }
      else if (mqtt_event->topic_len == 20) // IMM/I2/DAMess/Metric
      {
        my_json = cJSON_Parse(mqtt_event->data);
        da_message.timestamp = cJSON_GetObjectItem(my_json, "Timestamp")->valuestring;
        da_message.command = cJSON_GetObjectItem(my_json, "Command")->valueint;
        sprintf(message_text, "%s,%d", da_message.timestamp, da_message.command);
        if (da_message.command == Reboot)
        {
          ESP_LOGI("Restart", "DA Mess");
          esp_restart();
        }

        write_to_sd(message_text, CURRENT_STATUS_FILE);
        ESP_LOGI(TAG, "Change Mold");
        // TWDT
        esp_task_wdt_reset();
      }
      else if (mqtt_event->topic_len == 22) // IMM/I2/SyncTime/Metric
      {
        my_json = cJSON_Parse(mqtt_event->data);
        struct tm timesync;
        timesync.tm_year = cJSON_GetObjectItem(my_json, "Year")->valueint;
        timesync.tm_mon = cJSON_GetObjectItem(my_json, "Month")->valueint;
        timesync.tm_mday = cJSON_GetObjectItem(my_json, "Day")->valueint;
        timesync.tm_hour = cJSON_GetObjectItem(my_json, "Hour")->valueint;
        timesync.tm_min = cJSON_GetObjectItem(my_json, "Min")->valueint;
        timesync.tm_sec = cJSON_GetObjectItem(my_json, "Sec")->valueint;

        ESP_LOGI("A", "%d, %d , %d, %d, %d, %d", timesync.tm_year, timesync.tm_mon, timesync.tm_mday, timesync.tm_hour, timesync.tm_min, timesync.tm_sec);
        set_clock(&timesync);
        error_time_local = 0; // Tat co bao loi thoi gian
        // TWDT
        esp_task_wdt_reset();
      }
    }
    else
    {
      // TWDT
      esp_task_wdt_reset();
    }
  }
}
/*----------------------------------------------------------------------*/
static void app_notifications(void *arg)
{
  uint32_t command = 0;
  bool power_on = false;
  char message_text[500];
  char message_mqtt[500];

  esp_mqtt_client_config_t mqttConfig = {
      .uri = "mqtt://52.231.118.126:1883",
      // .username = CONFIG_USER_NAME,
      //.password = CONFIG_USER_PASSWORD,
      .disable_clean_session = false,
      .task_stack = 8192,
      .reconnect_timeout_ms = 30000,
      .lwt_topic = MACHINE_LWT_TOPIC,
      .lwt_msg = "Lost connect to device",
      .lwt_qos = 0,
      .disable_auto_reconnect = false
      // .keepalive = 200
  };

  // esp_task_wdt_reset();

  // TWDT
  // esp_task_wdt_add(NULL);
  // Publish RTC fail message
  if (ds3231_init_desc(&rtc_i2c, I2C_NUM_0, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO) != ESP_OK)
  {
    error_rtc = true;
    char mess_fb[200];
    sprintf(mess_fb, "{%cMess%c:%d}", 34, 34, RTCfail);
    // if (esp_mqtt_client_publish(client, FEEDBACK_TOPIC, mess_fb, 0, 1, 0) == -1)
    // {
    //   esp_mqtt_client_enqueue(client, FEEDBACK_TOPIC, mess_fb, 0, 1, 0, 1);
    // }
    ESP_LOGE(pcTaskGetTaskName(0), "Could not init device descriptor.");
  }
  else
  {
    esp_err_t err = ds3231_get_time(&rtc_i2c, &local_time);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "RTC error");
      error_rtc = true;
      char mess_fb[200];
      sprintf(mess_fb, "{%cMess%c:%d}", 34, 34, RTCfail);
      // if (esp_mqtt_client_publish(client, FEEDBACK_TOPIC, mess_fb, 0, 1, 0) == -1)
      // {
      //   esp_mqtt_client_enqueue(client, FEEDBACK_TOPIC, mess_fb, 0, 1, 0, 1);
      // }
      // Cai dat thoi gian trong truong hop module RTC co van de
      local_time.tm_year = 2022;
      local_time.tm_mon = 4;
      local_time.tm_mday = 20;
      local_time.tm_hour = 9;
      local_time.tm_min = 0;
      local_time.tm_sec = 0;
      inject_time.tm_year = 2022;
      inject_time.tm_mon = 4;
      inject_time.tm_mday = 20;
      inject_time.tm_hour = 9;
      inject_time.tm_min = 0;
      inject_time.tm_sec = 0;
      idle_time.tm_year = 2022;
      idle_time.tm_mon = 4;
      idle_time.tm_mday = 20;
      idle_time.tm_hour = 9;
      idle_time.tm_min = 0;
      idle_time.tm_sec = 0;
    }
    ESP_LOGI("TAG", "Get time OK %04d-%02d-%02dT%02d:%02d:%02d", local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour,
             local_time.tm_min, local_time.tm_sec);
  }
  // TWDT
  // esp_task_wdt_delete(NULL);
  client = esp_mqtt_client_init(&mqttConfig);
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
  while (1)
  {
    xTaskNotifyWait(0, 0, &command, portMAX_DELAY);
    // TWDT
    esp_task_wdt_add(NULL);
    switch (command)
    {
    case WIFI_CONNEECTED:
      esp_mqtt_client_start(client);
      // TWDT
      esp_task_wdt_delete(NULL);
      break;
    case MQTT_CONNECTED:
      xTaskCreate(mqtt_mess_task, "MQTT mess", 2048 * 2, NULL, 9, NULL);
      esp_mqtt_client_subscribe(client, DA_MESSAGE_TOPIC, 1);
      esp_mqtt_client_subscribe(client, CONFIGURATION_TOPIC, 1);
      esp_mqtt_client_subscribe(client, SYNC_TIME_TOPIC, 0);
      sprintf(message_mqtt, "[{%cname%c: %cmachineStatus%c,%cvalue%c: %d,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}]", 34, 34, 34, 34, 34, 34, Connected, 34, 34, 34, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour,
              local_time.tm_min, local_time.tm_sec, 34);
      // sprintf(message_mqtt,"[{%cTimestamp%c:%c%04d-%02d-%02dT%02d:%02d:%02d%c,%cmachineStatus%c:%d}]",34,34,34,local_time.tm_year, local_time.tm_mon,local_time.tm_mday, local_time.tm_hour,
      // local_time.tm_min, local_time.tm_sec,34,34,34,Connected);
      if (esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 0) == -1)
      {
        esp_mqtt_client_enqueue(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 0, 1);
      }
      // TWDT
      esp_task_wdt_delete(NULL);
      break;

    case INITIATE_SETUP:
      sprintf(message_text, "%04d-%02d-%02dT%02d:%02d:%02d,%d", local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, PowerOn);
      write_to_sd(message_text, CURRENT_STATUS_FILE);
      // TWDT
      esp_task_wdt_delete(NULL);
      break;

    default:
      // TWDT
      esp_task_wdt_delete(NULL);
      break;
    }
  }
}
/*----------------------------------------------------------------------*/
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  esp_task_wdt_add(NULL);
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
    xTaskNotify(taskHandle, SYSTEM_READY, eSetValueWithOverwrite);
    ESP_LOGI(TAG, "connecting...\n");
    // TWDT
    esp_task_wdt_reset();
    esp_task_wdt_delete(NULL);
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    ESP_LOGI("Wifi", "disconnected\n");
    reconnect_time++;
    if (reconnect_time < CONFIG_ESP_MAXIMUM_RETRY)
    {
      esp_wifi_connect();
      // ESP_LOGI("Wifi","Event Wifi Start reconnect after 5 min");
      // TWDT
      esp_task_wdt_reset();
      esp_task_wdt_delete(NULL);
    }
    // else if (reconnect_time >= CONFIG_ESP_MAXIMUM_RETRY)
    // {
    //   if (boot_to_reconnect == false)
    //   {
    //     boot_to_reconnect = true;
    //     xTimerStart(soft_timer_handle_7, 10);
    //     ESP_LOGI("Wifi", "Boot after 1h");
    //     // TWDT
    //     esp_task_wdt_reset();
    //   }
    //   esp_mqtt_client_stop(client);
    //   xTimerStart(soft_timer_handle_6, 10);
    //   // TWDT
    //   esp_task_wdt_reset();
    //   esp_task_wdt_delete(NULL);
    // }
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ESP_LOGI("Wifi", "Got ip\n");
    boot_to_reconnect = false;
    // xTimerStop(soft_timer_handle_6, 10);
    xTimerStop(soft_timer_handle_7, 10);

    xTaskNotify(taskHandle, WIFI_CONNEECTED, eSetValueWithOverwrite);
    // TWDT
    esp_task_wdt_reset();
    esp_task_wdt_delete(NULL);
  }
  else
  {
    // TWDT
    esp_task_wdt_reset();
    esp_task_wdt_delete(NULL);
  }
}
/*----------------------------------------------------------------------*/
void wifiInit()
{
  esp_task_wdt_add(NULL);

  ESP_ERROR_CHECK(esp_netif_init());                // Lib inheritance
  ESP_ERROR_CHECK(esp_event_loop_create_default()); // Lib inheritance

  esp_netif_t *my_sta = esp_netif_create_default_wifi_sta(); // Lib inheritance

  // Static IP address

  // esp_netif_dhcpc_stop(my_sta);

  esp_netif_ip_info_t ip_info;

  IP4_ADDR(&ip_info.ip, 10, 84, 20, 203); // 192, 168, 1, 207
  IP4_ADDR(&ip_info.gw, 10, 84, 20, 1);   // 192, 168, 1, 207

  IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

  // esp_netif_set_ip_info(my_sta, &ip_info);

  // Lib inheritance
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // TWDT
  esp_task_wdt_delete(NULL);

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_got_ip));
  // TWDT
  esp_task_wdt_add(NULL);

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = CONFIG_WIFI_SSID,
          .password = CONFIG_WIFI_PASSWORD,
          /* Setting a password implies station will connect to all security modes including WEP/WPA.
           * However these modes are deprecated and not advisable to be used. Incase your Access point
           * doesn't support WPA2, these mode can be enabled by commenting below line */
          .threshold.authmode = WIFI_AUTH_WPA2_PSK,

          .pmf_cfg = {
              .capable = true,
              .required = false},
      },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  // if wifi initializing OK, then feeding WDT
  esp_task_wdt_reset();
  esp_task_wdt_delete(NULL);
}
void app_main()
{
  // NVS init
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated and needs to be erased
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }

  error_sd_card = false;
  error_rtc = false;
  boot_to_reconnect = false;

  // Set up gpio input
  button_events = button_init(PIN_BIT(GPIO_INPUT_FINE_GRINDER) |
                              PIN_BIT(GPIO_INPUT_CYCLE_2));

  // Set up Hardware timer (phuc vu cho viec do thoi gian chu ky ep)
  timer_init_isr(TIMER_GROUP_0, TIMER_0, false, 0);
  timer_init_isr(TIMER_GROUP_0, TIMER_1, false, 0);
  timer_pause(TIMER_GROUP_0, TIMER_0);
  timer_pause(TIMER_GROUP_0, TIMER_1);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0x00000000ULL);

  // Set up Software timer (phuc vu cho viec nhu cac timer alarm)
  soft_timer_handle_1 = xTimerCreate(RECONNECT_BROKER_TIMER, pdMS_TO_TICKS(100000), false, (void *)1, &vSoftTimerCallback);
  soft_timer_handle_3 = xTimerCreate(INITIATE_TASK_TIMER, pdMS_TO_TICKS(5000), false, (void *)3, &vSoftTimerCallback);   // Timer Initiate
  soft_timer_handle_5 = xTimerCreate(STATUS_TIMER, pdMS_TO_TICKS(900000), false, (void *)5, &vSoftTimerCallback);        // Timer xac dinh trang thai Idle
  soft_timer_handle_6 = xTimerCreate(RECONNECT_TIMER, pdMS_TO_TICKS(3000), false, (void *)6, &vSoftTimerCallback);       // Timer scan wifi
  soft_timer_handle_7 = xTimerCreate(BOOT_CONNECT_TIMER, pdMS_TO_TICKS(3600000), false, (void *)7, &vSoftTimerCallback); // Timer Reboot to connect
  xTimerStart(soft_timer_handle_5, 10);
  xTimerStart(soft_timer_handle_3, 10);

  // Cai dat cycle info trong truong hop chua nhan duoc tin nhan config
  cfg_info.moldId = MOLD_ID_DEFAULT;
  cfg_info.productId = MOLD_ID_DEFAULT;
  cfg_info.cycle_cfg = 0;

  // Initializing the task watchdog subsystem with an interval of 2 seconds
  esp_task_wdt_init(2, true);

  // Ket noi Wifi
  wifiInit();

  // Khoi dong cac Task
  xTaskCreate(app_notifications, "App logic", 2048 * 4, NULL, 5, &taskHandle);  // Core1
  xTaskCreatePinnedToCore(gpio_task, "GPIO Task", 2048 * 4, NULL, 10, NULL, 0); // Core0

  mqtt_mess_events = xQueueCreate(10, sizeof(esp_mqtt_event_handle_t));

  //   while (1)
  // {
  //   char text[100];
  //   ds3231_get_time(&rtc_i2c, &local_time);

  //   sprintf(text, "%04d-%02d-%02dT%02d:%02d:%02d\n",
  //               local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec);

  //       printf(text);
  //       vTaskDelay(500/portTICK_PERIOD_MS);
  // }
}

static void initiate_task(void *arg)
{

  if (error_rtc == false)
  {
    ds3231_get_time(&rtc_i2c, &local_time);
    ds3231_get_time(&rtc_i2c, &inject_time);
  }

  if ((local_time.tm_year < 2022) || (local_time.tm_year > 2035) || (local_time.tm_mon > 12) || (local_time.tm_mday > 31))
  {
    local_time.tm_year = 2022;
    local_time.tm_mon = 4;
    local_time.tm_mday = 20;
    char mess_fb[200];
    sprintf(mess_fb, "{%cMess%c:%d}", 34, 34, SychTime);
    // if (esp_mqtt_client_publish(client, FEEDBACK_TOPIC, mess_fb, 0, 1, 0) == -1)
    // {
    //   esp_mqtt_client_enqueue(client, FEEDBACK_TOPIC, mess_fb, 0, 1, 0, 1);
    // }
    error_rtc = true; // Bat co bao loi thoi gian~
    ESP_LOGE("TAG", "The time on RTC board is not exactly --> Set default day");
  }

  remain_time = 10000;
  int64_t sub_time = 40000000;
  struct tm previousday;

  get_previous_day(&previousday, &local_time);

  ESP_LOGI(TAG, "previous %04d-%02d-%02d", previousday.tm_year, previousday.tm_mon, previousday.tm_mday);

  ESP_LOGI(TAG, "today %04d-%02d-%02d %02d:%02d:%02d", local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec);
  int shift = 1;
  int ofset = 0;
  if ((local_time.tm_hour > 18) || ((local_time.tm_hour == 18) && (local_time.tm_min >= 45)))
  {
    shift = 2;
    alarm_time.tm_hour = 6;
    alarm_time.tm_min = 45;
    alarm_time.tm_sec = 0;
    ESP_LOGI(TAG, "1");
  }
  else if ((local_time.tm_hour < 6) || ((local_time.tm_hour == 6) && (local_time.tm_min <= 44)))
  {
    shift = 2;
    ofset = 1;
    ESP_LOGI(TAG, "2");
    alarm_time.tm_hour = 6;
    alarm_time.tm_min = 45;
    alarm_time.tm_sec = 0;
  }
  else
  {
    alarm_time.tm_hour = 18;
    alarm_time.tm_min = 45;
    alarm_time.tm_sec = 0;
    ESP_LOGI(TAG, "3");
  }
  remain_time = calculate_diff_time(local_time, alarm_time);

  struct stat file_status_stat;
  struct stat file_cycle_stat;
  // Define file name to write
  // if (ofset == 0) // file name co ngay la ngay hom nay
  // {
  //   sprintf(CURRENT_STATUS_FILE, "/sdcard/s%d%02d%02d%02d.csv", shift, local_time.tm_mday, local_time.tm_mon, local_time.tm_year % 2000);
  //   if (stat(CURRENT_STATUS_FILE, &file_status_stat) != 0)
  //    // write_to_sd("Timestamp,machineStatus", CURRENT_STATUS_FILE);

  //   sprintf(CURRENT_CYCLE_FILE, "/sdcard/c%d%02d%02d%02d.csv", shift, local_time.tm_mday, local_time.tm_mon, local_time.tm_year % 2000);
  //   if (stat(CURRENT_CYCLE_FILE, &file_cycle_stat) != 0)
  //    // write_to_sd("Timestamp,CycleTime,OpenTime,Mode,CounterShot,MoldId,ProductId,SetCycle", CURRENT_CYCLE_FILE);
  //   else // Neu nhu file do co roi chung to ca do da chay roi, nhu vay can load cycle_id len
  //     load_value_nvs(&cycle_id, "cycle_id");
  // }
  // else // file name co ngay la ngay hom qua
  // {
  //   sprintf(CURRENT_STATUS_FILE, "/sdcard/s%d%02d%02d%02d.csv", shift, previousday.tm_mday, previousday.tm_mon, previousday.tm_year % 2000);
  //   if (stat(CURRENT_STATUS_FILE, &file_status_stat) != 0)
  //     //write_to_sd("Timestamp,machineStatus", CURRENT_STATUS_FILE);

  //   sprintf(CURRENT_CYCLE_FILE, "/sdcard/c%d%02d%02d%02d.csv", shift, previousday.tm_mday, previousday.tm_mon, previousday.tm_year % 2000);
  //   if (stat(CURRENT_CYCLE_FILE, &file_cycle_stat) != 0)
  //     write_to_sd("Timestamp,CycleTime,OpenTime,Mode,CounterShot,MoldId,ProductId,SetCycle", CURRENT_CYCLE_FILE);
  //   else // Neu nhu file do co roi chung to ca do da chay roi, nhu vay can load cycle_id len
  //     load_value_nvs(&cycle_id, "cycle_id");
  // }
  if (remain_time > 40000000)
  {
    remain_time = remain_time - sub_time;
    reboot_timer_flag = 1;
    soft_timer_handle_2 = xTimerCreate(SHIFT_REBOOT_TIMER, pdMS_TO_TICKS(40000000), false, (void *)2, &vSoftTimerCallback);
  }
  else
  {
    reboot_timer_flag = 2;
    soft_timer_handle_2 = xTimerCreate(SHIFT_REBOOT_TIMER, pdMS_TO_TICKS(remain_time), false, (void *)2, &vSoftTimerCallback);
  }

  ESP_LOGI("Remain time", "time %lld", remain_time);
  xTimerStart(soft_timer_handle_2, 10);

  // Set an alarm to restart esp32
  xTaskNotify(taskHandle, INITIATE_SETUP, eSetValueWithOverwrite);
  // remain_time = remain_time - sub_time;
  // Delete task
  delete_task();
}
