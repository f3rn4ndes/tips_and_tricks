#include <Arduino.h>
#include "driver/gpio.h"

const gpio_num_t kS0Pin = GPIO_NUM_27;
const gpio_num_t kS1Pin = GPIO_NUM_26;
const gpio_num_t kS2Pin = GPIO_NUM_33;
const gpio_num_t kS3Pin = GPIO_NUM_32;

const gpio_num_t kEPin = GPIO_NUM_25;

const gpio_num_t kProcessPin = GPIO_NUM_2;

const uint32_t kSerialBaudRate = 115200;

const uint32_t kDelayStartMs = 1000;
const uint32_t kDelayLoopMs = 1000;

const uint32_t kChannelsSize = 16;

const uint32_t kMultiplexSelector[kChannelsSize] =
    {0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8};

const uint32_t kMultiplexSelectorAlternate[kChannelsSize] =
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

const uint32_t kMockReadTimeUs = 200;

void IRAM_ATTR SetPinLevel(uint8_t pin, bool level);

void IRAM_ATTR WriteBus(uint8_t value);

void IRAM_ATTR MockReadDevice(uint32_t read_time_us);

void setup()
{
  delay(kDelayStartMs);
  Serial.begin(kSerialBaudRate);
  // Set the pins as outputs
  gpio_pad_select_gpio(kS0Pin);
  gpio_set_direction(kS0Pin, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(kS1Pin);
  gpio_set_direction(kS1Pin, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(kS2Pin);
  gpio_set_direction(kS2Pin, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(kS3Pin);
  gpio_set_direction(kS3Pin, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(kEPin);
  gpio_set_direction(kEPin, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(kProcessPin);
  gpio_set_direction(kProcessPin, GPIO_MODE_OUTPUT);

  delay(kDelayStartMs);

  Serial.printf("\n----------------------------------\n");
  Serial.printf("Start Application\n\n");
}

void loop()
{
  uint32_t check_time = 0;
  // Test by writing values 0 to 15
  Serial.printf("Start to Read Device\n");
  check_time = micros();
  SetPinLevel(kProcessPin, false);
  for (uint32_t i = 0; i < kChannelsSize; i++)
  {
    WriteBus(kMultiplexSelectorAlternate[i]);
    MockReadDevice(kMockReadTimeUs);
  }
  SetPinLevel(kProcessPin, true);
  check_time = micros() - check_time;
  Serial.printf("Time to read mock device (us): %lu \n", check_time);
  delay(kDelayLoopMs);
}

void IRAM_ATTR WriteBus(uint8_t value)
{
  // Set kEPin low (enable)
  SetPinLevel(kEPin, false);

  // Write the value to the bus
  SetPinLevel(kS0Pin, value & 0x01);
  SetPinLevel(kS1Pin, value & 0x02);
  SetPinLevel(kS2Pin, value & 0x04);
  SetPinLevel(kS3Pin, value & 0x08);

  // Set kEPin high (disable)
  SetPinLevel(kEPin, true);
}

void IRAM_ATTR MockReadDevice(uint32_t read_time_us)
{
  delayMicroseconds(read_time_us);
}

// Helper function to set a GPIO pin to a specific level
void IRAM_ATTR SetPinLevel(uint8_t pin, bool level)
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
