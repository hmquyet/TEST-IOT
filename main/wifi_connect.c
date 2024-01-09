#include "wifi_connect.h"
#include "main.h"



void wifiInit(void)
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