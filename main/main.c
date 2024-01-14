#include "main.h"

static void delete_task()
{
  vTaskDelete(NULL);
}

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

void restart_esp()
{
  cycle_id = 0;
  save_value_nvs(&cycle_id, "cycle_id");
  esp_restart();
}
/*----------------------------------------------------------------------*/
static void gpio_task(void *arg)
{
    // sdcard_mount();
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



  cycle_id = 0;
  int hardwaretimer;

  while (1)
  {
    esp_task_wdt_add(NULL);
    if (xQueueReceive(button_events, &ev, 1000 / portTICK_PERIOD_MS))
    {

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
        esp_mqtt_client_publish(client, FINE_GRINDER_STATUS_TOPIC, message_mqtt, 0, 1, 1);

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
        esp_mqtt_client_publish(client, FINE_GRINDER_STATUS_TOPIC, message_mqtt, 0, 1, 1);
        printf(message_text);
        ds3231_get_time(&rtc_i2c, &inject_time);
        sprintf(message_mqtt, "{%cname%c: %cLastFineGrinderTime%c,%cvalue%c: %f,%ctimestamp%c: %c%04d-%02d-%02dT%02d:%02d:%02d%c}",
                34, 34, 34, 34, 34, 34, last_fine_grinder_time, 34, 34, 34, inject_time.tm_year, inject_time.tm_mon, inject_time.tm_mday, inject_time.tm_hour, inject_time.tm_min, inject_time.tm_sec, 34);
        esp_mqtt_client_publish(client, FINE_GRINDER_LAST_TIME_TOPIC, message_mqtt, 0, 1, 1);
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

    /*NHỚ BẬT LẠI*/
    // xTaskCreate(initiate_task, "Alarm task", 2048 * 2, NULL, 4, initiate_taskHandle);

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

static void app_notifications(void *arg)
{
  uint32_t command = 0;
  bool power_on = false;
  char message_text[500];
  char message_mqtt[500];
 // xTaskCreatePinnedToCore(stm32f1_task, "stm32f1 Task", 2048 , NULL, 10, NULL, 0); // Core0

  // if (stm32f1_init(&rtc_i2c, I2C_NUM_0, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO) == ESP_OK)
  // {
  //    printf("INIT I2C STM32F1 OK\n");
  //    xTaskCreatePinnedToCore(stm32f1_task, "stm32f1 Task", 2048 , NULL, 10, NULL, 0); // Core0
   
  // }
  // else{
  //     printf("INIT I2C STM32F1 error\n");
  // }

  //mqtt_app_start();
 while (1)
 {
  /* code */
 }
 
    

  // // Publish RTC fail message
  // if (ds3231_init_desc(&rtc_i2c, I2C_NUM_0, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO) != ESP_OK)
  // {
  //   error_rtc = true;
  //   char mess_fb[200];
  //   sprintf(mess_fb, "{%cMess%c:%d}", 34, 34, RTCfail);

  //   ESP_LOGE(pcTaskGetTaskName(0), "Could not init device descriptor.");
  // }
  // else
  // {
  //   esp_err_t err = ds3231_get_time(&rtc_i2c, &local_time);
  //   if (err != ESP_OK)
  //   {
  //     ESP_LOGE(TAG, "RTC error");
  //     error_rtc = true;
  //     char mess_fb[200];
  //     sprintf(mess_fb, "{%cMess%c:%d}", 34, 34, RTCfail);

  //     // Cai dat thoi gian trong truong hop module RTC co van de
  //     local_time.tm_year = 2022;
  //     local_time.tm_mon = 4;
  //     local_time.tm_mday = 20;
  //     local_time.tm_hour = 9;
  //     local_time.tm_min = 0;
  //     local_time.tm_sec = 0;
  //     inject_time.tm_year = 2022;
  //     inject_time.tm_mon = 4;
  //     inject_time.tm_mday = 20;
  //     inject_time.tm_hour = 9;
  //     inject_time.tm_min = 0;
  //     inject_time.tm_sec = 0;
  //     idle_time.tm_year = 2022;
  //     idle_time.tm_mon = 4;
  //     idle_time.tm_mday = 20;
  //     idle_time.tm_hour = 9;
  //     idle_time.tm_min = 0;
  //     idle_time.tm_sec = 0;
  //   }
  //   ESP_LOGI("TAG", "Get time OK %04d-%02d-%02dT%02d:%02d:%02d", local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour,
  //            local_time.tm_min, local_time.tm_sec);
  // }
  // TWDT
   esp_task_wdt_delete(NULL);
  


}

/*----------------------------------------------------------------------*/
void app_main()
{

esp_task_wdt_init(2, true);
  
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
  soft_timer_handle_1 = xTimerCreate(RECONNECT_BROKER_TIMER, pdMS_TO_TICKS(2000), false, (void *)1, &vSoftTimerCallback);// timer reset mqtt connect
  soft_timer_handle_3 = xTimerCreate(INITIATE_TASK_TIMER, pdMS_TO_TICKS(5000), false, (void *)3, &vSoftTimerCallback); // Timer Initiate
  soft_timer_handle_5 = xTimerCreate(STATUS_TIMER, pdMS_TO_TICKS(900000), false, (void *)5, &vSoftTimerCallback);      // Timer xac dinh trang thai Idle
  soft_timer_handle_6 = xTimerCreate(RECONNECT_TIMER, pdMS_TO_TICKS(3000), false, (void *)6, &vSoftTimerCallback);     // Timer scan wifi
  soft_timer_handle_7 = xTimerCreate(BOOT_CONNECT_TIMER, pdMS_TO_TICKS(10000), false, (void *)7, &vSoftTimerCallback); // Timer Reboot to connect
  xTimerStart(soft_timer_handle_5, 10);
  xTimerStart(soft_timer_handle_3, 10);

  // Cai dat cycle info trong truong hop chua nhan duoc tin nhan config
  cfg_info.moldId = MOLD_ID_DEFAULT;
  cfg_info.productId = MOLD_ID_DEFAULT;
  cfg_info.cycle_cfg = 0;

  // Initializing the task watchdog subsystem with an interval of 2 seconds
  
    //stm32f1_init(&rtc_i2c, I2C_NUM_0, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO);
 //vTaskDelay(2000 / portTICK_PERIOD_MS);
  // Ket noi Wifi, MQTT
   wifiInit();
   
  

     //sdcard_mount();
    // write_to_sd(message_text, MACHINE_STATUS_FILE);
 //stm32f1_uart_config();
//stm32f1_reciever_uart(data_stm32f1);

  /*NHỚ BẬT LẠI*/
  // Khoi dong cac Task
  //task có uxPriority là 5 sẽ được ưu tiên hơn 
  //xTaskCreate(app_notifications, "App logic", 2048 * 4, NULL, 3, &taskHandle);  // Core1
   //xTaskCreate(gpio_task, "GPIO Task", 2048 * 4, NULL, 5, &taskHandle);  // Core1
  
  // mqtt_mess_events = xQueueCreate(10, sizeof(esp_mqtt_event_handle_t));
}

 

// void stm32f1_task(void *arg)
// {

//     while (1)
//     {
//       esp_err_t err_i2c_stm32f1 = stm32f1_get_data(&rtc_i2c, data_stm32f1, sizeof(data_stm32f1));
//       if ( err_i2c_stm32f1 != ESP_OK)
//       {     
        
//       }
//       else{
//         printf(data_stm32f1);
//       }
     

//     //vTaskDelay(2000 / portTICK_PERIOD_MS);
     
//     }
   
// }


static void initiate_task(void *arg)
{

  if (error_rtc == false)
  {
    ds3231_get_time(&rtc_i2c, &local_time);
    ds3231_get_time(&rtc_i2c, &inject_time);
  }

  if ((local_time.tm_year < 2022) || (local_time.tm_year > 2035) || (local_time.tm_mon > 12) || (local_time.tm_mday > 31))
  {
    local_time.tm_year = 2024;
    local_time.tm_mon = 1;
    local_time.tm_mday = 1;
    char mess_fb[200];
    sprintf(mess_fb, "{%cMess%c:%d}", 34, 34, SychTime);

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
