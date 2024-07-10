#pragma once

#include <Arduino.h>
#include "isignal.hpp"

class SignalAlpha : public ISignal
{
public:
    SignalAlpha(uint8_t pin);
    void Init(void) override;
    void Start(void) override;

private:
    uint8_t pin_;
};
