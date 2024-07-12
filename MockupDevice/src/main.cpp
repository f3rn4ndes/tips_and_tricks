#include <Arduino.h>
#include "driver/gpio.h"

const gpio_num_t kDataPin = GPIO_NUM_27;
const gpio_num_t kClockPin = GPIO_NUM_26;
const gpio_num_t kProcessPin = GPIO_NUM_2;

const uint32_t kSerialBaudRate = 115200;

const uint32_t kDelayStartMs = 1000;
const uint32_t kDelayLoopMs = 1000;
const uint32_t kDelayBetweenFramesUs = 125;

const uint32_t kChannelsSize = 8;

const uint32_t kFrameSizeBits = 16;

const uint16_t kFrameDataMask = 0x0FFF;

const uint32_t kDelayBitNs = 10;

const uint32_t kDelayRisingClockLine = 1;

void IRAM_ATTR SetPinLevel(uint32_t pin, boolean level);

void IRAM_ATTR WriteChannel(uint16_t address, uint16_t value);

void IRAM_ATTR DelayNanoSeconds(uint32_t value);

void setup()
{
  delay(kDelayStartMs);
  Serial.begin(kSerialBaudRate);
  // Set the pins as outputs
  gpio_pad_select_gpio(kDataPin);
  gpio_set_direction(kDataPin, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(kClockPin);
  gpio_set_direction(kClockPin, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(kProcessPin);
  gpio_set_direction(kProcessPin, GPIO_MODE_OUTPUT);

  delay(kDelayStartMs);

  Serial.printf("\n----------------------------------\n");
  Serial.printf("Start Application\n\n");

  SetPinLevel(kClockPin, false);
  SetPinLevel(kClockPin, false);
}

void loop()
{
  uint16_t channel_value = 0xAA55;

  // Test by writing values 0 to 15
  Serial.printf("Start to Generate Data\n");
  SetPinLevel(kProcessPin, false);
  for (uint16_t i = 0; i < kChannelsSize; i++)
  {
    WriteChannel(i, channel_value);
    delayMicroseconds(kDelayBetweenFramesUs);
    // channel_value ^= 0xFFFF;
  }
  SetPinLevel(kProcessPin, true);
  delay(kDelayLoopMs);
}

void IRAM_ATTR WriteChannel(uint16_t address, uint16_t value)
{
  uint16_t tx_frame = (value & kFrameDataMask) | (address << 12);

  // Serial.printf("Address: %04X, Value: %04X, Frame: %04X\n", address, value, tx_frame);
  // delay(kDelayLoopMs);

  // Serial.printf("Frame: ");
  for (uint32_t i = 0; i < kFrameSizeBits; i++)
  {
    // uint8_t bit_value = (tx_frame & 0x8000) > 0 ? 1 : 0;
    // Serial.printf("%d", bit_value);
    SetPinLevel(kDataPin, (tx_frame & 0x8000) > 0 ? true : false);
    tx_frame <<= 1;
    DelayNanoSeconds(kDelayRisingClockLine);
    SetPinLevel(kClockPin, true);
    DelayNanoSeconds(kDelayBitNs);
    SetPinLevel(kClockPin, false);
    DelayNanoSeconds(kDelayBitNs - kDelayRisingClockLine);
  }
  // Serial.printf("\n");
  // delay(kDelayLoopMs);
}

void IRAM_ATTR DelayNanoSeconds(uint32_t value)
{
  for (uint32_t i = 0; i < value; i++)
    ;
}

// Helper function to set a GPIO pin to a specific level
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
