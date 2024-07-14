#include <Arduino.h>
#include "FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include <freertos/semphr.h>

#define UART_NUM UART_NUM_0
#define TX_PIN UART_PIN_NO_CHANGE // Default TX pin for UART0
#define RX_PIN UART_PIN_NO_CHANGE // Default RX pin for UART0
#define BUF_SIZE (2048)

#define TIME_CRITICAL_TASK_STACK_SIZE 8192
#define TIME_CRITICAL_TASK_PRIORITY configMAX_PRIORITIES - 2

#define SERIAL_TASK_STACK_SIZE 8192
#define SERIAL_TASK_PRIORITY 0

#define GPIO_Set(x) REG_WRITE(GPIO_OUT_W1TS_REG, 1 << x)
#define GPIO_Clear(x) REG_WRITE(GPIO_OUT_W1TC_REG, 1 << x)
#define GPIO_IN_Read(x) REG_READ(GPIO_IN_REG) & (1 << x)

#define kClockPin GPIO_NUM_27
#define kDataPin GPIO_NUM_26

#define kProcessPin GPIO_NUM_2

#define kSignalPin GPIO_NUM_19

const uint32_t kSerialBaudRate = 576000;

const uint32_t kDelayStartMs = 1000;
const uint32_t kDelayLoopMs = 1000;

const uint32_t kClockLineMaxSize = 16;

const uint32_t kClockTimeout = 500000000;

const uint32_t kWdtRefreshInSeconds = 5;

const uint32_t kWdtTimeout = 100000;

// globals

TaskHandle_t timeCriticalTaskHandle = NULL;
TaskHandle_t serialTaskHandle = NULL;

uint16_t transmitData = 0;
boolean dataReady = false;

uint8_t clockLineCounter = 0;
uint32_t clockTimeout = 0;

uint32_t wdt_timeout_counter = 0;

char transmitBuffer[4];

void IRAM_ATTR SetPinLevel(uint32_t pin, boolean level);

inline uint32_t IRAM_ATTR clocks(void);

void IRAM_ATTR TransmitBuffer(void);

void IRAM_ATTR timeCriticalTask(void *pvParameters);

void IRAM_ATTR serialTask(void *pvParameters);

void InitUartDma(void);

void IRAM_ATTR WriteUartDma(const uint8_t *data, size_t length);

void setup()
{
  delay(kDelayStartMs);
  Serial.begin(kSerialBaudRate);
  // InitUartDma();

  // Set the pins as outputs
  gpio_pad_select_gpio(kProcessPin);
  gpio_set_direction(kProcessPin, GPIO_MODE_OUTPUT);

  gpio_pad_select_gpio(kSignalPin);
  gpio_set_direction(kSignalPin, GPIO_MODE_OUTPUT);

  // // Set the pins as inputs
  gpio_pad_select_gpio(kDataPin);
  gpio_set_direction(kDataPin, GPIO_MODE_INPUT);

  gpio_pad_select_gpio(kClockPin);
  gpio_set_direction(kClockPin, GPIO_MODE_INPUT);

  clockLineCounter = 0;
  clockTimeout = 0;
  dataReady = false;

  transmitBuffer[2] = 0x00;
  transmitBuffer[3] = 0x00;

  GPIO_Clear(kSignalPin);
  GPIO_Clear(kProcessPin);

  delay(kDelayStartMs);

  Serial.printf("\n----------------------------------\n");
  Serial.printf("Start Application\n\n");

  // Create the time-critical task pinned to core 1
  xTaskCreatePinnedToCore(
      timeCriticalTask,
      "timeCriticalTask",
      TIME_CRITICAL_TASK_STACK_SIZE,
      NULL,
      TIME_CRITICAL_TASK_PRIORITY,
      &timeCriticalTaskHandle,
      1 // Pin to core 1
  );

  // Create the serial task pinned to core 0
  // xTaskCreatePinnedToCore(
  //     serialTask,
  //     "serialTask",
  //     SERIAL_TASK_STACK_SIZE,
  //     NULL,
  //     SERIAL_TASK_PRIORITY,
  //     &serialTaskHandle,
  //     0 // Pin to core 0
  // );
}

void loop()
{
  vTaskDelete(NULL);
}

inline void IRAM_ATTR TransmitBuffer(void)
{
  // SetPinLevel(kProcessPin, false);
  char buffer_tx[4];
  buffer_tx[0] = (transmitData >> 8) & 0xFF;
  buffer_tx[1] = transmitData & 0xFF;
  buffer_tx[2] = 0x0A;
  buffer_tx[3] = 0;
  Serial.write(buffer_tx);
  // SetPinLevel(kProcessPin, true);
  // Serial.flush();
}

void IRAM_ATTR timeCriticalTask(void *pvParameters)
{

  register int level, old_level = 0, data_line = 0;

  register uint32_t check_timeout = 0;

  register uint32_t check_timeout_frame_size = 0;

  register uint16_t transmit_data = 0;

  while (true)
  {

    level = GPIO_IN_Read(kClockPin);

    if ((level != old_level) && (old_level == 0))
    {
      data_line = GPIO_IN_Read(kDataPin);
      transmit_data <<= 1;
      if (!clockLineCounter)
      {
        GPIO_Set(kSignalPin);
        check_timeout_frame_size = micros();
        transmit_data = 0;
        clockTimeout = clocks();
        clockLineCounter = 1;
        if (data_line)
          transmit_data = 1;
      }
      else
      {
        clockTimeout = clocks() - clockTimeout;
        if (clockTimeout > kClockTimeout)
        {
          transmit_data = 0;
          clockLineCounter = 0;
          // clockTimeout = clocks();
          // clockLineCounter = 0;
          // if (data_line)
          // {
          //   transmit_data = 1;
          // }

          delayMicroseconds(10);
        }
        else
        {
          clockTimeout = clocks();
          clockLineCounter++;
          if (data_line)
          {
            transmit_data += 1;
          }
          if (clockLineCounter >= kClockLineMaxSize)
          {
            clockLineCounter = 0;
            GPIO_Clear(kSignalPin);
            GPIO_Set(kProcessPin);
            transmitBuffer[0] = (transmit_data >> 8) & 0xFF;
            transmitBuffer[1] = transmit_data & 0xFF;
            Serial.write(transmitBuffer);
            GPIO_Clear(kProcessPin);
          }
        }
      }
    }
    old_level = level;
  }
}

void InitUartDma(void)
{
  uart_config_t uart_config = {
      .baud_rate = kSerialBaudRate,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  uart_param_config(UART_NUM, &uart_config);
  uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);
  uart_set_mode(UART_NUM, UART_MODE_UART);
}

void IRAM_ATTR SetPinLevel(uint32_t pin, boolean level)
{
  if (level)
  {
    if (pin < 32)
    {
      GPIO.out_w1ts = (1 << pin);
    }
    else
    {
      GPIO.out1_w1ts.val = (1 << (pin - 32));
    }
  }
  else
  {
    if (pin < 32)
    {
      GPIO.out_w1tc = (1 << pin);
    }
    else
    {
      GPIO.out1_w1tc.val = (1 << (pin - 32));
    }
  }
}

inline uint32_t IRAM_ATTR clocks()
{
  uint32_t ccount;
  asm volatile("rsr %0, ccount" : "=a"(ccount));
  return ccount;
}
