#include <Arduino.h>

// Pin Mapping - Digital Output
#define S0 27
#define S1 26
#define S2 33
#define S3 32
#define EN 25

// constants

const uint32_t kInitialDelay = 1000;
const uint32_t kSerialBaudRate = 115200;

const uint32_t kLoopDelay = 3000;

const uint32_t kLedDelay = 200;

const uint32_t kPinsMask1 = (1 << S0) | (1 << S1);
const uint32_t kPinsMask2 = (1 << (S2 - 32)) | (1 << (S3 - 32));
const uint32_t kPinsMaskEn = (1 << EN);

#define USE_LEDS 0

#define MUX_SELECTORS 4

#define INSOLE_CHANNELS (uint8_t)16

int selectorMatrix[MUX_SELECTORS] = {S3, S2, S1, S0};

float insoleChannelsValues[INSOLE_CHANNELS];

// Binary Values to Multiplex Selectors
byte multiplexMatrix[INSOLE_CHANNELS][MUX_SELECTORS] = {
    {0, 0, 0, 0}, // 0  em decimal
    {0, 0, 0, 1}, // 1  em decimal
    {0, 0, 1, 0}, // 2  em decimal
    {0, 0, 1, 1}, // 3  em decimal
    {0, 1, 0, 0}, // 4  em decimal
    {0, 1, 0, 1}, // 5  em decimal
    {0, 1, 1, 0}, // 6  em decimal
    {0, 1, 1, 1}, // 7  em decimal
    {1, 1, 1, 1}, // 15 em decimal 1111
    {1, 1, 1, 0}, // 14 em decimal 1110
    {1, 1, 0, 1}, // 13 em decimal 1101
    {1, 1, 0, 0}, // 12 em decimal 1100
    {1, 0, 1, 1}, // 11 em decimal 1011
    {1, 0, 1, 0}, // 10 em decimal 1010
    {1, 0, 0, 1}, // 9  em decimal 1001
    {1, 0, 0, 0}, // 8  em decimal 1000
};

void setupPins();

void writePinsUsingFor();

void writePinsDirectly();

void setup()
{

  delay(kInitialDelay);
  Serial.begin(kSerialBaudRate);
  setupPins();
  Serial.println("Comparing For Loop x Directly Writting");
}

void loop()
{

  unsigned long startTime = micros();
  writePinsUsingFor();
  unsigned long endTime = micros();
  unsigned long duration = endTime - startTime;

  Serial.println("-----------------------------------------");
  Serial.print("Time taken to write pins using for loop: ");
  Serial.print(duration);
  Serial.println(" microsseconds");

  Serial.println("-----------------------------------------");
  startTime = micros();
  writePinsDirectly();
  endTime = micros();
  duration = endTime - startTime;
  Serial.print("Time taken to write pins using directly mode: ");
  Serial.print(duration);
  Serial.println(" microsseconds");

  delay(kLoopDelay);
}

void setupPins()
{
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(S0, false);
  digitalWrite(S1, false);
  digitalWrite(S2, false);
  digitalWrite(S3, false);
  digitalWrite(EN, true);
}

void writePinsUsingFor()
{
  for (int _counterChannel = 0; _counterChannel < INSOLE_CHANNELS; _counterChannel++)
  {
    digitalWrite(EN, true);
    for (int _counterSelector = 0; _counterSelector < MUX_SELECTORS; _counterSelector++)
    {
      digitalWrite(selectorMatrix[_counterSelector], multiplexMatrix[_counterChannel][_counterSelector]);
    }

    digitalWrite(EN, false);
#if (USE_LEDS == 1)
    delay(kLedDelay);
#endif
  }
}

void writePinsDirectly()
{
  for (uint32_t _counterChannel = 0; _counterChannel < INSOLE_CHANNELS; _counterChannel++)
  {
    uint32_t values1 = ((_counterChannel & 0x01) << S0) |
                       ((_counterChannel & 0x02) << (S1 - 1));

    uint32_t values2 = (((_counterChannel & 0x04) >> 2) << (S2 - 32)) |
                       (((_counterChannel & 0x08) >> 3) << (S3 - 32));

    // Disable Mux
    GPIO.out_w1ts = kPinsMaskEn;

    // Clear the bits first
    GPIO.out_w1tc = kPinsMask1;
    GPIO.out1_w1tc.val = kPinsMask2;

    // Set the bits
    GPIO.out_w1ts = values1;
    GPIO.out1_w1ts.val = values2;

    // Enable Mux
    GPIO.out_w1tc = kPinsMaskEn;

#if (USE_LEDS == 1)
    delay(kLedDelay);
    Serial.printf("CounterChannel: %d\n", _counterChannel);
    Serial.printf("values1: %X\n", values1);
    Serial.printf("values2: %X\n", values2);
#endif
  }
}
