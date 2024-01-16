
#include "SorftTimer.h"
#include "main.h"


static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
  BaseType_t high_task_awoken = pdFALSE;
  return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}


void vSoftTimerCallback(TimerHandle_t xTimer)
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

void timer_init_isr(int group, int timer, bool auto_reload, int timer_interval_sec)
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