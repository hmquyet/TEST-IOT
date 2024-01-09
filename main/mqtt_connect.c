#include "mqtt_connect.h"
#include "main.h"

 void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  mqtt_event_handler_cb(event_data);
}

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

 void mqtt_mess_task(void *arg)
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

        //write_to_sd(message_text, CURRENT_STATUS_FILE);
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

        ESP_LOGI("AAAAAAAA", "%d, %d , %d, %d, %d, %d", timesync.tm_year, timesync.tm_mon, timesync.tm_mday, timesync.tm_hour, timesync.tm_min, timesync.tm_sec);
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

void mqtt_app_start(void)
{
esp_mqtt_client_config_t mqttConfig = {
      .uri = CONFIG_BROKER_URL,
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

    client = esp_mqtt_client_init(&mqttConfig);

}
