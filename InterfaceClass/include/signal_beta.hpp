#pragma once

#include <Arduino.h>
#include "isignal.hpp"

class SignalBeta : public ISignal
{
public:
    SignalBeta(uint8_t pin);
    void Init(void) override;
    void Start(void);
    void CustomStart(void);

private:
    uint8_t pin_;
};
