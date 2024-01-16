#include "main.h"
#include "task_esp.h"

void gpio_task(void *arg)
{
  // sdcard_mount();
  double injection_time = 0;
  double injection_time_1 = 0;

  double last_idle_time = 0;
  double last_operation_time = 0;
  double last_delay_time = 0;

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

      if ((ev.pin == GPIO_INPUT_STATUS) && (ev.event == BUTTON_DOWN)) //cắm 3v3 rút ra /// vào DI là OV, gia công
      {

        ESP_LOGI("GPIO", "Event BUTTON_DOWN");

        xTimerStop(soft_timer_handle_5, 10); // Timer check idle status
        xTimerStart(soft_timer_handle_5, 10);

        timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_1, &last_delay_time);

        // ds3231_get_time(&rtc_i2c, &local_time);
        // sprintf(message_mqtt, "{\"name\": \"MachineStatus\",\"value\": %d,\"timestamp\": \"%04d-%02d-%02dT%02d:%02d:%02d\"}",
        //         Running, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec);
        // esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 1);
        // printf(message_text);
        printf("%d\n",Running);
        // TWDT
        esp_task_wdt_delete(NULL);

        // TWDT
        esp_task_wdt_add(NULL);
        // TWDT
        esp_task_wdt_reset();
        // }
      }


      else if ((ev.pin == GPIO_INPUT_STATUS) && (ev.event == BUTTON_UP)) //cắm GND rút ra///  DI là 24V, dừng gia công 
      {
        xTimerStop(soft_timer_handle_5, 10); // Timer check idle status
        xTimerStart(soft_timer_handle_5, 10);

        timer_pause(TIMER_GROUP_0, TIMER_1); // Dung timer dem thoi gian IDLE_time
        timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_1, &last_idle_time);

        timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0x00000000ULL); // Set cho timer ve gia tri 0
        timer_start(TIMER_GROUP_0, TIMER_1);

        ESP_LOGI("GPIO", "Event BUTTON_UP ");
        // if (last_running_time > 1.0)
        // {
        // ds3231_get_time(&rtc_i2c, &local_time);
        // sprintf(message_mqtt, "{\"name\": \"MachineStatus\",\"value\": %d,\"timestamp\": \"%04d-%02d-%02dT%02d:%02d:%02d\"}",
        //         Idle, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec);
       // esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 1);
       // printf(message_text);
       printf("%d\n",Idle);


        //Tính idle time
        // ds3231_get_time(&rtc_i2c, &inject_time);
        // sprintf(message_mqtt, "{\"name\": \"OperationTime\",\"value\": %f,\"timestamp\": \"%04d-%02d-%02dT%02d:%02d:%02d\"}",
        //         last_idle_time, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec);
        //esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 1);
        //printf(message_text);
        
        printf("%f\n",last_idle_time);

      //Tính operation time
        last_operation_time = fabs(last_idle_time - last_delay_time);

        // ds3231_get_time(&rtc_i2c, &inject_time);
        // sprintf(message_mqtt, "{\"name\": \"OperationTime\",\"value\": %f,\"timestamp\": \"%04d-%02d-%02dT%02d:%02d:%02d\"}",
        //         last_operation_time, local_time.tm_year, local_time.tm_mon, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec);
        // esp_mqtt_client_publish(client, MACHINE_STATUS_TOPIC, message_mqtt, 0, 1, 1);
        // printf(message_text);
         printf("%f\n",last_operation_time);
        // TWDT
        esp_task_wdt_delete(NULL);

        // TWDT
        esp_task_wdt_add(NULL);
        // TWDT
        esp_task_wdt_reset();
        // }
      }

      else if ((ev.pin == GPIO_INPUT_DEFECTIVE) && (ev.event == BUTTON_UP)) //cắm GND rút ra///  
      {
         
      }
      else if ((ev.pin == GPIO_INPUT_DEFECTIVE) && (ev.event == BUTTON_DOWN)) //cắm 3v3 rút ra/// 
      {
      
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