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


#include "RTC.h"
#include "SDCard.h"
#include "mqtt_connect.h"
#include "wifi_connect.h"
#include "button.h"
#include "stm32f1uart.h"


#define TAG "TEST-IOT"
#define STORAGE_NAMESPACE "esp32_storage" // save variable to nvs storage

#define TIMER_DIVIDER (16)                           //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

// Define pin to read Cycle period, Open time, Machine's status
#define GPIO_INPUT_FINE_GRINDER 36 // 36
#define GPIO_INPUT_CYCLE_2 39 // 39




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
  char *name;
  char *value;
  char *timestamp;
  
} data_message_t;

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
data_message_t data_message;
cfg_info_t cfg_info;

struct tm local_time;
struct tm inject_time;
struct tm alarm_time;
struct tm idle_time;

static const uint32_t WIFI_CONNEECTED = BIT1;
static const uint32_t MQTT_CONNECTED = BIT2;
// static const uint32_t MQTT_PUBLISHED = BIT3;
static const uint32_t SYSTEM_READY = BIT4;
static const uint32_t INITIATE_SETUP = BIT5;
// static const uint32_t SYNC_TIME_DONE = BIT6;
// static const uint32_t SYNC_TIME_FAIL = BIT7;

// const char* CYCLE_TIME_TOPIC = "IMM/I2/Metric/CycleMessage";
static const char *MACHINE_STATUS_TOPIC = "IMM/I2/Metric/MachineStatus";
static const char *INJECTION_CYCLE_TOPIC = "IMM/I2/Metric/InjectionCycle";
static const char *INJECTION_TIME_TOPIC = "IMM/I2/Metric/InjectionTime";


static const char *FINE_GRINDER_STATUS_TOPIC = "FALAB/MACHANICAL MACHINES/FineGrinder/Metric/FineGrinderStatus";
static const char *FINE_GRINDER_LAST_TIME_TOPIC = "FALAB/MACHANICAL MACHINES/FineGrinder/Metric/LastFineGrinderTime";


static char MACHINE_STATUS_FILE[50] = "/sdcard/status.csv";
static char MACHINE_RUNNING_TIME_FILE[50] = "/sdcard/running.csv";
static char MACHINE_GRINDING_TIME_FILE[50] = "/sdcard/grinding.csv";

static const char *MACHINE_LWT_TOPIC = "IMM/I2/Metric/LWT";
static const char *DA_MESSAGE_TOPIC = "IMM/I2/Metric/DAMess";
static const char *CONFIGURATION_TOPIC = "IMM/I2/Metric/ConfigMess";
static const char *SYNC_TIME_TOPIC = "IMM/I2/Metric/SyncTime";
static const char *FEEDBACK_TOPIC = "IMM/I2/Metric/Feedback";
static const char *MOLD_ID_DEFAULT = "NotConfigured";
static const char *RECONNECT_BROKER_TIMER = "RECONNECT_BROKER_TIMER";
static const char *OPEN_CHECK_TIMER = "OPEN_CHECK_TIMER";
static const char *INITIATE_TASK_TIMER = "INITIATE_TASK_TIMER";
static const char *SHIFT_REBOOT_TIMER = "SHIFT_REBOOT_TIMER";
static const char *STATUS_TIMER = "STATUS_TIMER";
static const char *RECONNECT_TIMER = "RECONNECT_TIMER";
static const char *BOOT_CONNECT_TIMER = "BOOT_CONNECT_TIMER";

 int cycle_id;
 int reconnect_time;
 int error_time_local;
int reboot_timer_flag;
 int64_t remain_time;
 bool error_sd_card;
 bool error_rtc;
 
bool boot_to_reconnect;
 bool idle_trigger;
 cJSON *my_json;
static char CURRENT_CYCLE_FILE[50] = "/sdcard/c1090422.csv";
 static char  CURRENT_STATUS_FILE[50] = "/sdcard/s1090422.csv";
  char nameData[100];
  char valueData[100]; 
  char data_stm32f1[100];
void stm32f1_task(void *arg);

