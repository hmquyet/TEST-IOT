

#include "SDCard.h"
#include "main.h"


/*----------------------------------------------------------------------*/
/**
 * @brief Write content to file in SD card in append+ mode
 *
 * @param content Content to write
 * @param file_path File path to write
 */
/*----------------------------------------------------------------------*/
/**
 * @brief Mount SD card using SPI bus
 *
 */
void write_to_sd(char content[], char file_path[])
{
  FILE *f = fopen(file_path, "a+");
  if (f == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for writing --> Restart ESP");
    esp_restart();
    return;
  }
  int i = 0;
  while (content[i] != NULL)
    i++;

  char buff[i + 1];
  for (int j = 0; j < i + 1; j++)
  {
    buff[j] = content[j];
  }
  fprintf(f, buff);
  fprintf(f, "\n");
  fclose(f);
  ESP_LOGI(TAG, "File written");
}

void sdcard_mount()
{
  /*sd_card part code*/
  esp_vfs_fat_sdmmc_mount_config_t mount_config =
      {
          .format_if_mount_failed = true,
          .max_files = 5,
          .allocation_unit_size = 16 * 1024};
  sdmmc_card_t *card;
  const char mount_point[] = MOUNT_POINT;
  ESP_LOGI(TAG, "Initializing SD card");

  ESP_LOGI(TAG, "Using SPI peripheral");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  spi_bus_config_t bus_cfg = {
      .mosi_io_num = PIN_NUM_MOSI,
      .miso_io_num = PIN_NUM_MISO,
      .sclk_io_num = PIN_NUM_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
  };

  esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize bus.");
  }

  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = PIN_NUM_CS;
  slot_config.host_id = host.slot;
  esp_err_t err = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (err != ESP_OK)
  {
    error_sd_card = true;
    char mess_fb[200];
    sprintf(mess_fb, "{%cMess%c:%d}", 34, 34, SDcardfail);

    if (err == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize the card. "
                    "Make sure SD card lines have pull-up resistors in place.");
    }
  }
  else if (err == ESP_OK)
  {
    sdmmc_card_print_info(stdout, card);
   
  }
}

 esp_err_t unmount_card(const char *base_path, sdmmc_card_t *card)
{
  esp_err_t err = esp_vfs_fat_sdcard_unmount(base_path, card);

  ESP_ERROR_CHECK(err);

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  err = spi_bus_free(host.slot);

  ESP_ERROR_CHECK(err);

  return err;
}