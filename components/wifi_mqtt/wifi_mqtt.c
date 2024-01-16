#include "wifi_mqtt.h"

#include "main.h"
static const char *TAGWIFI = "WIFI_ESP";

void wifiInit(void)
{

  esp_event_loop_create_default();
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
  esp_task_wdt_add(NULL);
  wifi_config_t wifi_config = {
      .sta = {
          .ssid = EXAMPLE_ESP_WIFI_SSID,
          .password = EXAMPLE_ESP_WIFI_PASS,
          .threshold.authmode = WIFI_AUTH_WPA2_PSK},
  };

  esp_netif_init();
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  esp_wifi_start();
ESP_LOGI(TAG, "wifi_init finished.");
  esp_task_wdt_reset();
  esp_task_wdt_delete(NULL);
}

esp_err_t wifi_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
  switch (event_id)
  {
  case WIFI_EVENT_STA_START:
    esp_wifi_connect();
    reconnect_time = 0;
    ESP_LOGI(TAGWIFI, " to Tryingconnect with Wi-Fi\n");
    break;

  case WIFI_EVENT_STA_CONNECTED:
    boot_to_reconnect = false;
    // xTimerStop(soft_timer_handle_6, 10);
    xTimerStop(soft_timer_handle_7, 10);
  
    ESP_LOGI(TAGWIFI, "Wi-Fi connected\n");
    ESP_LOGI(TAGWIFI, " Connect to SSID:%s, password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    mqtt_app_start();
     
    break;

  case IP_EVENT_STA_GOT_IP:
    ESP_LOGI(TAGWIFI, "got ip: starting Wifi Client\n");

    break;

  case WIFI_EVENT_STA_DISCONNECTED:
    
    
    reconnect_time++;
    if (reconnect_time < MAX_RETRY)
    {
      ESP_LOGI(TAGWIFI, "wifi disconnected: Retrying Wi-Fi\n");
      esp_wifi_connect();
    }
    else if (reconnect_time >= MAX_RETRY)
    {
      if (boot_to_reconnect == false)
      {
        boot_to_reconnect = true;
        xTimerStart(soft_timer_handle_7, 10);
        ESP_LOGI("Wifi", "Boot after 1 min");
        // TWDT
        esp_task_wdt_reset();
      }
      esp_mqtt_client_stop(client);
      //xTimerStart(soft_timer_handle_6, 10);
      // TWDT
      esp_task_wdt_reset();
      esp_task_wdt_delete(NULL);
    }
    
    break;

  default:
    break;
  }
  return ESP_OK;
}



static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        xTimerStop(soft_timer_handle_1, 10);
        // xTaskCreate(mqtt_mess_task, "MQTT mess", 2048 * 2, NULL, 9, NULL);
        // esp_mqtt_client_subscribe(event->client, "your/topic", 0);
        esp_mqtt_client_subscribe(event->client,MATERIAL_CODE_TOPIC, 0);

        break;

    case MQTT_EVENT_DISCONNECTED:

        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        xTimerStart(soft_timer_handle_1, 10);

        break;
    case MQTT_EVENT_SUBSCRIBED:

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    case MQTT_EVENT_DATA:

        ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        // ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        // ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

        memcpy(topic_buffer, event->topic, event->topic_len);
        topic_buffer[event->topic_len] = '\0'; // Null-terminate the string

        memcpy(data_buffer, event->data, event->data_len);
        data_buffer[event->data_len] = '\0'; // Null-terminate the string

        printf("TOPIC=%s\n", topic_buffer);
        printf("DATA=%s\n", data_buffer);

        mqtt_data_processing(data_buffer);

        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(void)
{

    ESP_LOGI(TAG, "STARTING MQTT");
    esp_mqtt_client_config_t mqttConfig = {
        .uri = "mqtt://20.249.217.5:1883"};

    client = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_data_processing(char *mqttJson)
{
    char message_text[1000];
    cJSON *json = cJSON_Parse(mqttJson);
    cJSON *json_array = cJSON_GetArrayItem(json, 0);

    cJSON *name = cJSON_GetObjectItem(json_array, "Name");
    cJSON *value = cJSON_GetObjectItem(json_array, "Value");
    cJSON *timestamp = cJSON_GetObjectItem(json_array, "Timestamp");
     //printf("vô đây r nha..........\n");

    sprintf(message_text, "Material Code,%s,Timestamp,%s",value->valuestring,timestamp->valuestring);
    write_to_sd(message_text, MATERIAL_CODE_FILE);
    stm32f1_tranmister_uart( name->valuestring,value->valuestring);

    // printf("Name: %s\n", name->valuestring);
    // printf("Value: %s\n", value->valuestring);
    // printf("Timestamp: %s\n", timestamp->valuestring);

    cJSON_Delete(json);
}
