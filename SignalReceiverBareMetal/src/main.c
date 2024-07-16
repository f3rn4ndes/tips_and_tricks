#include "esp_system.h"
#include "soc/uart_struct.h"
#include "soc/uart_reg.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h> // For strlen

#define UART0_TX_PIN (1) // GPIO1 (default TX pin for UART0)
#define UART0_BAUD_RATE (115200)
#define TAG "Bare-metal"

// Function to initialize UART0 for transmission
void uart_init()
{
    // Configure UART0 TX pin
    gpio_set_direction(UART0_TX_PIN, GPIO_MODE_OUTPUT);

    // Configure UART0 parameters
    WRITE_PERI_REG(UART_CLKDIV_REG(0), (APB_CLK_FREQ << 4) / UART0_BAUD_RATE);
    SET_PERI_REG_BITS(UART_CONF0_REG(0), UART_BIT_NUM, UART_BIT_NUM_8BITS, UART_BIT_NUM_S);
    CLEAR_PERI_REG_MASK(UART_CONF0_REG(0), UART_PARITY_EN_M);
    CLEAR_PERI_REG_MASK(UART_CONF0_REG(0), UART_STOP_BIT_NUM_M);
    SET_PERI_REG_BITS(UART_CONF0_REG(0), UART_STOP_BIT_NUM, UART_STOP_BITS_1, UART_STOP_BIT_NUM_S);
    CLEAR_PERI_REG_MASK(UART_CONF0_REG(0), UART_FLOW_CONF_MASK);
    SET_PERI_REG_MASK(UART_CONF0_REG(0), UART_TXD_BRK);
}

// Function to send data via UART0
void uart_send_data(const char *data, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        // Wait for TX FIFO to be not full
        while (UART0.status.txfifo_cnt >= 127)
            ;
        // Write byte to UART FIFO
        UART0.fifo.rw_byte = data[i];
    }
}

void main_loop()
{
    const char *test_str = "Hello, UART0!";
    while (1)
    {
        uart_send_data(test_str, strlen(test_str));
        for (volatile int i = 0; i < 1000000; i++)
            ; // Delay loop
    }
}

void app_main()
{
    // Disable the FreeRTOS scheduler to go bare-metal
    vTaskDelete(NULL);

    uart_init();
    ESP_LOGI(TAG, "UART0 initialized for transmission.");

    main_loop();
}
