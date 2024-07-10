#include "signal_beta.hpp"

SignalBeta::SignalBeta(uint8_t pin) : pin_(pin)
{
}

void SignalBeta::Init(void)
{
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, HIGH);
}

void SignalBeta::Start(void)
{
    for (uint8_t counter = 0; counter < 10; counter++)
    {
        delay(100);
        digitalWrite(pin_, LOW);
        delay(100);
        digitalWrite(pin_, HIGH);
    }
}

void SignalBeta::CustomStart(void)
{
    for (uint8_t counter = 0; counter < 10; counter++)
    {
        delay(100);
        digitalWrite(pin_, LOW);
        delay(100);
        digitalWrite(pin_, HIGH);
    }
}
