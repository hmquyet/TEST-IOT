#include "main.h"


void restart_esp()
{
  cycle_id = 0;
  save_value_nvs(&cycle_id, "cycle_id");
  esp_restart();
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
  button_events = button_init(PIN_BIT(GPIO_INPUT_STATUS) |
                              PIN_BIT(GPIO_INPUT_DEFECTIVE)|
                              PIN_BIT(GPIO_INPUT_MAINTENING)|
                              PIN_BIT(GPIO_INPUT_MALFUNCTION));

  // Set up Hardware timer (phuc vu cho viec do thoi gian chu ky ep)
  timer_init_isr(TIMER_GROUP_0, TIMER_0, false, 0);
  timer_init_isr(TIMER_GROUP_0, TIMER_1, false, 0);
  timer_pause(TIMER_GROUP_0, TIMER_0);
  timer_pause(TIMER_GROUP_0, TIMER_1);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0x00000000ULL);

  // Set up Software timer (phuc vu cho viec nhu cac timer alarm)
  soft_timer_handle_1 = xTimerCreate(RECONNECT_BROKER_TIMER, pdMS_TO_TICKS(2000), false, (void *)1, &vSoftTimerCallback); // timer reset mqtt connect
  soft_timer_handle_3 = xTimerCreate(INITIATE_TASK_TIMER, pdMS_TO_TICKS(5000), false, (void *)3, &vSoftTimerCallback);    // Timer Initiate
  soft_timer_handle_5 = xTimerCreate(STATUS_TIMER, pdMS_TO_TICKS(900000), false, (void *)5, &vSoftTimerCallback);         // Timer xac dinh trang thai Idle
  soft_timer_handle_6 = xTimerCreate(RECONNECT_TIMER, pdMS_TO_TICKS(3000), false, (void *)6, &vSoftTimerCallback);        // Timer scan wifi
  soft_timer_handle_7 = xTimerCreate(BOOT_CONNECT_TIMER, pdMS_TO_TICKS(10000), false, (void *)7, &vSoftTimerCallback);    // Timer Reboot to connect

  xTimerStart(soft_timer_handle_5, 10);
  xTimerStart(soft_timer_handle_3, 10);

  // Cai dat cycle info trong truong hop chua nhan duoc tin nhan config
  cfg_info.moldId = MOLD_ID_DEFAULT;
  cfg_info.productId = MOLD_ID_DEFAULT;
  cfg_info.cycle_cfg = 0;

  // Initializing the task watchdog subsystem with an interval of 2 seconds

  // stm32f1_init(&rtc_i2c, I2C_NUM_0, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO);
  // vTaskDelay(2000 / portTICK_PERIOD_MS);
  //  Ket noi Wifi, MQTT
 // wifiInit();

  // sdcard_mount();
  // write_to_sd(message_text, MACHINE_STATUS_FILE);
  // stm32f1_uart_config();
  // stm32f1_reciever_uart(data_stm32f1);

  /*NHỚ BẬT LẠI*/
  // Khoi dong cac Task
  // task có uxPriority là 5 sẽ được ưu tiên hơn
  // xTaskCreate(app_notifications, "App logic", 2048 * 4, NULL, 3, &taskHandle);  // Core1
   xTaskCreate(gpio_task, "GPIO Task", 2048 * 4, NULL, 5, &taskHandle);  // Core1
  // xTaskCreate(mqtt_mess_processing_task, "mqtt string Task", 2048 * 4, NULL, 3, &taskHandle);  // Core1
  //  mqtt_mess_events = xQueueCreate(10, sizeof(esp_mqtt_event_handle_t));
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

