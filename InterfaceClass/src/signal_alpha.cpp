#include "signal_alpha.hpp"

SignalAlpha::SignalAlpha(uint8_t pin) : pin_(pin)
{
}

void SignalAlpha::Init(void)
{
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
}

void SignalAlpha::Start(void)
{
    delay(1000);
    digitalWrite(pin_, HIGH);
}
