#include "mqtt_connect.h"
#include "main.h"


//  void mqtt_mess_task(void *arg)
// {
//   esp_mqtt_event_handle_t mqtt_event;
//   while (1)
//   {
//     char message_text[1000];
//     if (xQueueReceive(mqtt_mess_events, &mqtt_event, 1000 / portTICK_PERIOD_MS))
//     {
//       if (mqtt_event->topic_len == 24) // IMM/I2/ConfigMess/Metric
//       {
//         my_json = cJSON_Parse(mqtt_event->data);
//         cfg_info.timestamp = cJSON_GetObjectItem(my_json, "Timestamp")->valuestring;
//         cfg_info.moldId = cJSON_GetObjectItem(my_json, "MoldId")->valuestring;
//         cfg_info.productId = cJSON_GetObjectItem(my_json, "ProductId")->valuestring;
//         cfg_info.cycle_cfg = cJSON_GetObjectItem(my_json, "CycleTime")->valuedouble;
//         cfg_info.is_configured = true;
//        // ESP_LOGI(TAG, "Timestamp:%s, moldId:%s, productId:%s, Cycle:%f", cfg_info.timestamp, cfg_info.moldId, cfg_info.productId, cfg_info.cycle_cfg);
//         // TWDT
//         esp_task_wdt_reset();
//       }
//       else if (mqtt_event->topic_len == 20) // IMM/I2/DAMess/Metric
//       {
//         my_json = cJSON_Parse(mqtt_event->data);
//         da_message.timestamp = cJSON_GetObjectItem(my_json, "Timestamp")->valuestring;
//         da_message.command = cJSON_GetObjectItem(my_json, "Command")->valueint;
//         sprintf(message_text, "%s,%d", da_message.timestamp, da_message.command);
//         if (da_message.command == Reboot)
//         {
//           ESP_LOGI("Restart", "DA Mess");
//           esp_restart();
//         }

//         //write_to_sd(message_text, CURRENT_STATUS_FILE);
//         ESP_LOGI(TAG, "Change Mold");
//         // TWDT
//         esp_task_wdt_reset();
//       }
//       else if (mqtt_event->topic_len == 22) // IMM/I2/SyncTime/Metric
//       {
//         my_json = cJSON_Parse(mqtt_event->data);
//         struct tm timesync;
//         timesync.tm_year = cJSON_GetObjectItem(my_json, "Year")->valueint;
//         timesync.tm_mon = cJSON_GetObjectItem(my_json, "Month")->valueint;
//         timesync.tm_mday = cJSON_GetObjectItem(my_json, "Day")->valueint;
//         timesync.tm_hour = cJSON_GetObjectItem(my_json, "Hour")->valueint;
//         timesync.tm_min = cJSON_GetObjectItem(my_json, "Min")->valueint;
//         timesync.tm_sec = cJSON_GetObjectItem(my_json, "Sec")->valueint;

//        // ESP_LOGI("AAAAAAAA", "%d, %d , %d, %d, %d, %d", timesync.tm_year, timesync.tm_mon, timesync.tm_mday, timesync.tm_hour, timesync.tm_min, timesync.tm_sec);
//         set_clock(&timesync);
//         error_time_local = 0; // Tat co bao loi thoi gian
//         // TWDT
//         esp_task_wdt_reset();
//       }
//     }
//     else
//     {
//       // TWDT
//       esp_task_wdt_reset();
//     }
//   }
// }
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
        esp_mqtt_client_subscribe(event->client, "your/topic", 0);
        esp_mqtt_client_subscribe(event->client, "me/topic", 0);

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

ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        // // Copy MQTT topic and data to buffers
        // memcpy(topic_buffer, event->topic, event->topic_len);
        // topic_buffer[event->topic_len] = '\0'; // Null-terminate the string

        // memcpy(data_buffer, event->data, event->data_len);
        // data_buffer[event->data_len] = '\0'; // Null-terminate the string

        // // Print the character arrays
        // printf("TOPIC=%s\n", topic_buffer);
        // printf("DATA=%s\n", data_buffer);

        mqtt_mess_processing(event->data );

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


void mqtt_mess_processing(const char *mqttData){
       
        data_message.timestamp = cJSON_GetObjectItem(mqttData, "Timestamp")->valuestring;
        data_message.name = cJSON_GetObjectItem(mqttData, "Name")->valuestring;
        data_message.value = cJSON_GetObjectItem(mqttData, "Value")->valuestring;
        printf(data_message.value);
        printf(data_message.name);

}
