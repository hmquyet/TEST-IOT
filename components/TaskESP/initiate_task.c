#include "main.h"
#include "task_esp.h"

 void initiate_task(void *arg)
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

void app_notifications(void *arg)
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

  // mqtt_app_start();
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
