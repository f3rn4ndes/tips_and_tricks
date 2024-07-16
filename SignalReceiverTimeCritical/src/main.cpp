#pragma GCC optimize("O3")

#include <Arduino.h>
#include "FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "soc/gpio_struct.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include <freertos/semphr.h>
#include "soc/rtc_cntl_reg.h"
#include "soc/uart_reg.h"

#define UART_NUM UART_NUM_0
#define TX_PIN GPIO_NUM_1         // Default TX pin for UART0
#define RX_PIN UART_PIN_NO_CHANGE // Default RX pin for UART0
#define BUF_SIZE (2048)

#define TIME_CRITICAL_TASK_STACK_SIZE 8192
#define TIME_CRITICAL_TASK_PRIORITY configMAX_PRIORITIES - 1

#define SERIAL_TASK_STACK_SIZE 8192
#define SERIAL_TASK_PRIORITY 0

#define GPIO_Set(x) REG_WRITE(GPIO_OUT_W1TS_REG, 1 << x)
#define GPIO_Clear(x) REG_WRITE(GPIO_OUT_W1TC_REG, 1 << x)
#define GPIO_IN_Read(x) REG_READ(GPIO_IN_REG) & (1 << x)

#define kClockPin GPIO_NUM_27
#define kDataPin GPIO_NUM_26

#define kProcessPin GPIO_NUM_2

#define kSignalPin GPIO_NUM_19
#define kErrorPin GPIO_NUM_18

const uint32_t kSerialBaudRate = 576000;

const uint32_t kDelayStartMs = 1000;
const uint32_t kDelayLoopMs = 1000;

const uint32_t kClockLineMaxSize = 16;

const uint32_t kClockTimeout = 300000000;

const uint32_t kWdtRefreshInSeconds = 5;

const uint32_t kWdtTimeout = 100000;

uint32_t check_timeout = 0;

// globals

TaskHandle_t timeCriticalTaskHandle = NULL;

portMUX_TYPE criticalMutex = portMUX_INITIALIZER_UNLOCKED;

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
void IRAM_ATTR timeCriticalTaskB(void *pvParameters);

inline uint32_t IRAM_ATTR myMicros(void);

void IRAM_ATTR DisableScheduler();

void setup()
{
  delay(kDelayStartMs);
  Serial.begin(kSerialBaudRate);

  // Set the pins as outputs
  gpio_pad_select_gpio(kProcessPin);
  gpio_set_direction(kProcessPin, GPIO_MODE_OUTPUT);

  gpio_pad_select_gpio(kSignalPin);
  gpio_set_direction(kSignalPin, GPIO_MODE_OUTPUT);

  gpio_pad_select_gpio(kErrorPin);
  gpio_set_direction(kErrorPin, GPIO_MODE_OUTPUT);

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
  GPIO_Clear(kErrorPin);

  delay(kDelayStartMs);

  // Create the time-critical task pinned to core 1
  xTaskCreatePinnedToCore(
      timeCriticalTaskB,
      "timeCriticalTask",
      TIME_CRITICAL_TASK_STACK_SIZE,
      NULL,
      TIME_CRITICAL_TASK_PRIORITY,
      &timeCriticalTaskHandle,
      1 // Pin to core 1
  );
  delay(1000);
}

void loop()
{
  vTaskDelete(NULL);
  // configASSERT(uxCriticalNesting == 1000UL);
  // taskYIELD();
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

  register uint16_t transmit_data = 0;

  boolean check = false;

  // DisableScheduler();

  GPIO_Set(kSignalPin);
  GPIO_Set(kProcessPin);
  while (!check)
  {
    level = GPIO_IN_Read(kClockPin);
    if (!level)
      check = true;
  };
  check = false;
  while (!check)
  {
    level = GPIO_IN_Read(kClockPin);
    if (level)
      check = true;
  };
  delayMicroseconds(20);
  GPIO_Clear(kSignalPin);
  GPIO_Clear(kProcessPin);
  level = 0;
  old_level = 0;

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
          clockTimeout = clocks();
          clockLineCounter = 0;
          if (data_line)
          {
            transmit_data = 1;
          }
          level = 0;
          GPIO_Set(kErrorPin);
          delayMicroseconds(15);
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
            GPIO_Set(kProcessPin);
            transmitBuffer[0] = (transmit_data >> 8) & 0xFF;
            transmitBuffer[1] = transmit_data & 0xFF;
            Serial.write(transmitBuffer);
            GPIO_Clear(kSignalPin);
            GPIO_Clear(kProcessPin);
            GPIO_Clear(kErrorPin);
            level = 0;
          }
        }
      }
    }
    old_level = level;
  }
}

void IRAM_ATTR DisableScheduler()
{
  // Disable Task Watchdog Timer
  // esp_task_wdt_deinit();
  WRITE_PERI_REG(RTC_CNTL_WDTCONFIG0_REG, 0);
  // // Disable the main system watchdog timer
  WRITE_PERI_REG(TIMG_WDTCONFIG0_REG(0), 0);
  WRITE_PERI_REG(TIMG_WDTCONFIG0_REG(1), 0);

  // Disable Interrupt Watchdog Timer
  // esp_int_wdt_disable();
}

inline uint32_t IRAM_ATTR clocks()
{
  uint32_t ccount;
  asm volatile("rsr %0, ccount" : "=a"(ccount));
  return ccount;
}

inline uint32_t IRAM_ATTR myMicros()
{
  return clocks() / (240);
}

void IRAM_ATTR timeCriticalTaskB(void *pvParameters)
{

  register int level, old_level = 0, data_line = 0;

  register uint16_t transmit_data = 0;

  boolean check = false;

  GPIO_Set(kSignalPin);
  GPIO_Set(kProcessPin);
  while (!check)
  {
    level = GPIO_IN_Read(kClockPin);
    if (!level)
      check = true;
  };
  check = false;
  while (!check)
  {
    level = GPIO_IN_Read(kClockPin);
    if (level)
      check = true;
  };

  delayMicroseconds(20);
  GPIO_Clear(kSignalPin);
  GPIO_Clear(kProcessPin);
  level = 0;
  old_level = 0;

  clockTimeout = 0;

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
        clockTimeout = myMicros();
        transmit_data = 0;
        clockLineCounter = 1;
        if (data_line)
          transmit_data = 1;
      }
      else
      {
        clockLineCounter++;
        if (data_line)
        {
          transmit_data += 1;
        }
        if (clockLineCounter >= kClockLineMaxSize)
        {
          clockTimeout = myMicros() - clockTimeout;
          clockLineCounter = 0;
          GPIO_Set(kProcessPin);
          transmitBuffer[0] = (transmit_data >> 8) & 0xFF;
          transmitBuffer[1] = transmit_data & 0xFF;
          if (clockTimeout < 100)
            Serial.write(transmitBuffer);
          else
          {
            delayMicroseconds(10);
          }
          GPIO_Clear(kSignalPin);
          GPIO_Clear(kProcessPin);
          level = 0;
        }
      }
    }
    old_level = level;
  }
}
