#include <Arduino.h>
#include <SPI.h>
#include "driver/gpio.h"

const boolean kUsesDefaultSpiPins = false;

const boolean kUsesSpiClockDivider = false;

const uint32_t kSpiFrequency = 1536000;

const uint8_t kSpiSckPin = GPIO_NUM_27;
const uint8_t kSpiMosiPin = GPIO_NUM_26;
const uint8_t kSpiMisoPin = GPIO_NUM_33;
const uint8_t kSpiCsPin = GPIO_NUM_25;

const gpio_num_t kProcessPin = GPIO_NUM_2;

const uint32_t kSerialBaudRate = 115200;

const uint32_t kDelayStartMs = 1000;
const uint32_t kDelayLoopMs = 1000;
const uint32_t kDelayBetweenFramesUs = 125;

const uint32_t kChannelsSize = 8;
const uint16_t kFrameDataMask = 0x0FFF;

// globals
SPISettings spiSettings(kSpiFrequency, MSBFIRST, SPI_MODE0);

void SpiSetup(uint8_t spi_mosi_pin, uint8_t spi_miso_pin,
              uint8_t spi_sck_pin, uint8_t spi_cs_pin);

void IRAM_ATTR SetPinLevel(uint32_t pin, boolean level);

void IRAM_ATTR WriteChannel(uint16_t address, uint16_t value);

void IRAM_ATTR DelayNanoSeconds(uint32_t value);

void setup()
{
  delay(kDelayStartMs);
  Serial.begin(kSerialBaudRate);

  SpiSetup(kSpiMosiPin, kSpiMisoPin, kSpiSckPin, kSpiCsPin);

  // Set the pins as outputs
  gpio_pad_select_gpio(kProcessPin);
  gpio_set_direction(kProcessPin, GPIO_MODE_OUTPUT);

  delay(kDelayStartMs);

  Serial.printf("\n----------------------------------\n");
  Serial.printf("Start Application\n\n");
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
    if (i < (kChannelsSize - 1))
      delayMicroseconds(kDelayBetweenFramesUs);
  }
  SetPinLevel(kProcessPin, true);
  delay(kDelayLoopMs);
}

void IRAM_ATTR WriteChannel(uint16_t address, uint16_t value)
{
  uint16_t tx_frame = (value & kFrameDataMask) | (address << 12);

  SPI.beginTransaction(spiSettings);
  SPI.transfer16(tx_frame);
  SPI.endTransaction();
}

void SpiSetup(uint8_t spi_mosi_pin, uint8_t spi_miso_pin, uint8_t spi_sck_pin, uint8_t spi_cs_pin)
{
  if (kUsesDefaultSpiPins)
    SPI.begin();
  else
    SPI.begin(spi_sck_pin, spi_miso_pin, spi_mosi_pin, spi_cs_pin);

  if (kUsesSpiClockDivider)
  {
    uint32_t clockDiv = (80000000 + (kSpiFrequency - 1)) / kSpiFrequency; // Calculate the clock divider for 1.536 MHz

    // Set clock divisor
    SPI.setClockDivider((clockDiv << 12) | (clockDiv << 6) | (clockDiv));
  }
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
