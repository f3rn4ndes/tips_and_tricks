#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define SERIAL_TX_DIRECT 0
#define SERIAL_TX_PRINT 1
#define SERIAL_TX_MODE SERIAL_TX_DIRECT

#define GPIO_IN_Read(x) REG_READ(GPIO_IN_REG) & (1 << x)

#define kClockPin GPIO_NUM_27
#define kDataPin GPIO_NUM_26

#define kProcessPin GPIO_NUM_2

const uint32_t kSerialBaudRate = 576000;

const uint32_t kDelayStartMs = 1000;
const uint32_t kDelayLoopMs = 1000;

const uint32_t kClockLineMaxSize = 16;

const uint32_t kClockTimeout = 500000000;

// globals
uint16_t transmitData = 0;
boolean dataReady = false;

uint8_t clockLineCounter = 0;
uint32_t clockTimeout = 0;

void IRAM_ATTR SetPinLevel(uint32_t pin, boolean level);

inline uint32_t IRAM_ATTR clocks(void);

void IRAM_ATTR TransmitBuffer(void);

void setup()
{
  delay(kDelayStartMs);
  Serial.begin(kSerialBaudRate);

  // Set the pins as outputs
  gpio_pad_select_gpio(kProcessPin);
  gpio_set_direction(kProcessPin, GPIO_MODE_OUTPUT);

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
}

void loop()
{
  register int level, old_level = 0, data_line = 0;

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
          Serial.printf("%lu\n", clockTimeout);
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
            clockLineCounter = 0;
            TransmitBuffer();
          }
        }
      }
    }
    old_level = level;
  }
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
