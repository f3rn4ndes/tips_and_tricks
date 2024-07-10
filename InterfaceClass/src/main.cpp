#include <Arduino.h>
#include "isignal.hpp"
#include "signal_alpha.hpp"
#include "signal_beta.hpp"

ISignal *iSignalAlpha;
ISignal *iSignalBeta;

void setup()
{

  delay(1000);
  Serial.begin(115200);

  // Create objects
  iSignalAlpha = new SignalAlpha(GPIO_NUM_20);

  iSignalBeta = new SignalBeta(GPIO_NUM_21);

  // Init Objects
  iSignalAlpha->Init();
  iSignalBeta->Init();

  iSignalAlpha->Start();
  iSignalBeta->Start();
  

}

void loop()
{
  delay(1000);
}
