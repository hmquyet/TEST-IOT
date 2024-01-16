#include "main.h"
#include "task_esp.h"

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