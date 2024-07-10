#pragma once

#include <Arduino.h>

class ISignal
{
public:
    virtual void Init(void) = 0; // must be implemented
    virtual void Start(void) {}; // should not be implemented
};
