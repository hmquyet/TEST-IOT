
#include "stm32f1uart.h"
#include "main.h"

static const char *TAG_UART = "LORA_ESP";
int status = 1;
void stm32f1_uart_config()
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // Install UART driver, and get the queue.
    uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart2_queue, 0);
    uart_param_config(UART_NUM, &uart_config);
    // Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(UART_NUM, TX_GPIO_NUM, RX_GPIO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_enable_pattern_det_baud_intr(UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    // Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(UART_NUM, 20);
}

void stm32f1_tranmister_uart(char name[], char value[])
{
    char data_tranmister[300];

    snprintf(data_tranmister, sizeof(data_tranmister), "!DATA:%s:%s#",name,value);
    uart_write_bytes(UART_NUM, (const char *)data_tranmister, strlen(data_tranmister));
    printf(data_tranmister);

}
void processing_string_Receive_uart(char inputStrings[] ,char name[],char value[])
{
   char *colonPosition1 = strchr(inputStrings, ':');

    if (colonPosition1 != NULL) {
        // Tìm vị trí của dấu hai chấm (:) tiếp theo
        char *colonPosition2 = strchr(colonPosition1 + 1, ':');

        if (colonPosition2 != NULL) {
            // Tìm vị trí của ký tự kết thúc giá trị (trước dấu #)
            char *hashPosition = strchr(colonPosition2, '#');

            if (hashPosition != NULL) {
                // Tính toán độ dài của giá trị
                size_t valueLength = hashPosition - (colonPosition2 + 1);

                // Sao chép tên vào mảng name
                strncpy(name, colonPosition1 + 1, colonPosition2 - (colonPosition1 + 1));
                name[colonPosition2 - (colonPosition1 + 1)] = '\0';

                // Sao chép giá trị vào mảng value
                strncpy(value, colonPosition2 + 1, valueLength);
                value[valueLength] = '\0';
            }
        }
    }

    printf("name: %s\n",name);
    printf("value: %s\n",value);
}
void process_string_Tranmister(char name[],char value[], char outputStrings[], size_t outputSize )
	{
		snprintf(outputStrings, outputSize, "!DATA:%s:%s#",name,value);
}

void stm32f1_reciever_uart(char *data)
{

    size_t buffered_size;
    uint8_t *dtmp = (uint8_t *)malloc(RD_BUF_SIZE);
    for (;;)
    {
        // Waiting for UART event.

        if (xQueueReceive(uart2_queue, (void *)&event, (portTickType)portMAX_DELAY))
        {
            // Clear the memory block
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI("UART", "uart[%d] event:", UART_NUM);
            switch (event.type)
            {
            // Event of UART receving data
            case UART_DATA:
                vTaskDelay(100 / portTICK_RATE_MS);
                ESP_LOGI(TAG_UART, "[UART DATA]: %d", event.size);
                int len = uart_read_bytes(UART_NUM, dtmp, event.size, portMAX_DELAY);
                data = (char *)dtmp;
                printf(data);
                processing_string_Receive_uart(data,nameData,valueData);

                break;
            // Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG_UART, "hw fifo overflow");
                break;
            // Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG_UART, "ring buffer full");
                break;
            // Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI(TAG_UART, "uart rx break");
                break;
            // Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI(TAG_UART, "uart parity error");
                break;
            // Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(TAG_UART, "uart frame error");
                break;
            // UART_PATTERN_DET
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(UART_NUM, &buffered_size);
                int pos = uart_pattern_pop_pos(UART_NUM);
                ESP_LOGI("UART", "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1)
                {

                    uart_flush_input(UART_NUM);
                }

                else
                {
                    uart_read_bytes(UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                    uint8_t pat[PATTERN_CHR_NUM + 1];
                    memset(pat, 0, sizeof(pat));
                    uart_read_bytes(UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG_UART, "read data: %s", dtmp);
                    ESP_LOGI(TAG_UART, "read pat : %s", pat);
                }
                break;
            // Others
            default:
                ESP_LOGI("UART", "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}
