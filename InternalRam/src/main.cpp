#include <Arduino.h>
#include "esp_heap_caps.h"

#define LED_PIN 2

const uint8_t kS0Pin = GPIO_NUM_27;
const uint8_t kS1Pin = GPIO_NUM_26;
const uint8_t kS2Pin = GPIO_NUM_33;
const uint8_t kS3Pin = GPIO_NUM_32;
const uint8_t kEPin = GPIO_NUM_25;

const uint32_t kPinsMask1 = (1 << kS0Pin) | (1 << kS1Pin);
const uint32_t kPinsMask2 = (1 << (kS2Pin - 32)) | (1 << (kS3Pin - 32));

void NoCriticalCode();

void IRAM_ATTR CriticalCode();

void SelectChannel(uint8_t channel);

void IRAM_ATTR SelectChannelDirectly(uint32_t channel);

const uint8_t kSelectorMatrix[4] = {kS3Pin, kS2Pin, kS1Pin, kS0Pin};

// Binary Values to Multiplex Selectors
const uint8_t kMultiplexMatrix[16][4] = {
    {0, 0, 0, 0}, // 0  in decimal
    {0, 0, 0, 1}, // 1  in decimal
    {0, 0, 1, 0}, // 2  in decimal
    {0, 0, 1, 1}, // 3  in decimal
    {0, 1, 0, 0}, // 4  in decimal
    {0, 1, 0, 1}, // 5  in decimal
    {0, 1, 1, 0}, // 6  in decimal
    {0, 1, 1, 1}, // 7  in decimal
    {1, 1, 1, 1}, // 15 in decimal 1111
    {1, 1, 1, 0}, // 14 in decimal 1110
    {1, 1, 0, 1}, // 13 in decimal 1101
    {1, 1, 0, 0}, // 12 in decimal 1100
    {1, 0, 1, 1}, // 11 in decimal 1011
    {1, 0, 1, 0}, // 10 in decimal 1010
    {1, 0, 0, 1}, // 9  in decimal 1001
    {1, 0, 0, 0}, // 8  in decimal 1000
};

void setup()
{

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  pinMode(kS3Pin, OUTPUT);
  pinMode(kS2Pin, OUTPUT);
  pinMode(kS1Pin, OUTPUT);
  pinMode(kS0Pin, OUTPUT);

  digitalWrite(LED_PIN, false);
}

void loop()
{
  uint32_t check_time;

  check_time = micros();
  NoCriticalCode();
  check_time = micros() - check_time;
  Serial.printf("Time No Critical = %lu\n", check_time);
  check_time = micros();
  CriticalCode();
  check_time = micros() - check_time;
  Serial.printf("Time Critical = %lu\n", check_time);
  delay(2000);
}

void NoCriticalCode()
{
  for (int i = 0; i < 16; i++)
  {
    SelectChannel(i);
  }
}

void IRAM_ATTR CriticalCode()
{
  for (int i = 0; i < 16; i++)
  {
    SelectChannelDirectly(i);
  }
}

void SelectChannel(uint8_t channel)
{
  for (uint8_t multiplexer_selector = 0; multiplexer_selector < 4; multiplexer_selector++)
  {
    digitalWrite(kSelectorMatrix[multiplexer_selector], kMultiplexMatrix[channel][multiplexer_selector]);
  }
}

void IRAM_ATTR SelectChannelDirectly(uint32_t channel)
{
  uint32_t values1 = ((channel & 0x01) << kS0Pin) |
                     ((channel & 0x02) << (kS1Pin - 1));

  uint32_t values2 = (((channel & 0x04) >> 2) << (kS2Pin - 32)) |
                     (((channel & 0x08) >> 3) << (kS3Pin - 32));

  // Clear the bits first
  GPIO.out_w1tc = kPinsMask1;
  GPIO.out1_w1tc.val = kPinsMask2;

  // Set the bits
  GPIO.out_w1ts = values1;
  GPIO.out1_w1ts.val = values2;
}
