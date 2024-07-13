#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define TIME_CRITICAL_TASK_STACK_SIZE 8192
#define TIME_CRITICAL_TASK_PRIORITY configMAX_PRIORITIES - 1

#define SERIAL_TASK_STACK_SIZE 8192
#define SERIAL_TASK_PRIORITY 4

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

// globals

TaskHandle_t timeCriticalTaskHandle = NULL;
TaskHandle_t serialTaskHandle = NULL;
SemaphoreHandle_t uartMutex;

uint16_t transmitData = 0;
boolean dataReady = false;

uint8_t clockLineCounter = 0;
uint32_t clockTimeout = 0;

void IRAM_ATTR SetPinLevel(uint32_t pin, boolean level);

inline uint32_t IRAM_ATTR clocks(void);

void IRAM_ATTR TransmitBuffer(void);

void IRAM_ATTR timeCriticalTask(void *pvParameters);

void IRAM_ATTR serialTask(void *pvParameters);

void setup()
{
  delay(kDelayStartMs);
  Serial.begin(kSerialBaudRate);

  uartMutex = xSemaphoreCreateMutex(); // Create a mutex for UART access

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
  xTaskCreatePinnedToCore(
      serialTask,
      "serialTask",
      SERIAL_TASK_STACK_SIZE,
      NULL,
      SERIAL_TASK_PRIORITY,
      &serialTaskHandle,
      0 // Pin to core 0
  );
}

void loop()
{
  // vTaskDelete(NULL);
}

void IRAM_ATTR TransmitBuffer(void)
{
  SetPinLevel(kProcessPin, false);
  char buffer_tx[4];
  buffer_tx[0] = (transmitData >> 8) & 0xFF;
  buffer_tx[1] = transmitData & 0xFF;
  buffer_tx[2] = 0x0A;
  buffer_tx[3] = 0;
  Serial.write(buffer_tx);
  SetPinLevel(kProcessPin, true);
}

void IRAM_ATTR timeCriticalTask(void *pvParameters)
{
  int level, old_level = 0, data_line = 0;

  uint32_t check_timeout = 0;

  while (true)
  {
    level = GPIO_IN_Read(kClockPin);

    if ((level != old_level) && (old_level == 0))
    {
      data_line = GPIO_IN_Read(kDataPin);
      transmitData <<= 1;
      if (!clockLineCounter)
      {
        transmitData = 0;
        clockTimeout = clocks();
        clockLineCounter = 1;
        if (data_line)
          transmitData = 1;
      }
      else
      {
        clockTimeout = clocks() - clockTimeout;
        if (clockTimeout > kClockTimeout)
        {
          transmitData = 0;
          clockTimeout = clocks();
          clockLineCounter = 1;
          if (data_line)
          {
            transmitData = 1;
          }
        }
        else
        {
          clockLineCounter++;
          clockTimeout = clocks();
          if (data_line)
          {
            transmitData += 1;
          }
          if (clockLineCounter >= kClockLineMaxSize)
          {
            TransmitBuffer();
            level = old_level = 0;
            clockLineCounter = 0;
          }
        }
      }
    }
    old_level = level;
  }
}

void IRAM_ATTR serialTask(void *pvParameters)
{
  while (true)
  {
    if (xSemaphoreTake(uartMutex, portMAX_DELAY))
    {
      if (Serial.available() > 0)
      {
        int data = Serial.read();
        Serial.write(data);
      }
      xSemaphoreGive(uartMutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
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
