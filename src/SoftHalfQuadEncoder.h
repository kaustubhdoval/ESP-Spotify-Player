#ifndef SOFT_HALF_QUAD_ENCODER_H
#define SOFT_HALF_QUAD_ENCODER_H

#include <Arduino.h>

// ESP32-C3 has no PCNT peripheral (SOC_PCNT_SUPPORTED is unset), so the
// ESP32Encoder library cannot be used on the PCB. This reproduces its
// "half quad" decoding in software: both edges of pin A are counted, and
// the level of pin B at the time of the edge gives the direction.
class SoftHalfQuadEncoder
{
public:
  void attachHalfQuad(int aPin, int bPin)
  {
    instance = this;
    _aPin = aPin;
    _bPin = bPin;
    pinMode(aPin, INPUT_PULLUP);
    pinMode(bPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(aPin), isr, CHANGE);
  }

  int64_t getCount()
  {
    portENTER_CRITICAL(&_mux);
    int64_t result = _count;
    portEXIT_CRITICAL(&_mux);
    return result;
  }

  void setCount(int64_t value)
  {
    portENTER_CRITICAL(&_mux);
    _count = value;
    portEXIT_CRITICAL(&_mux);
  }

private:
  static void IRAM_ATTR isr()
  {
    if (!instance)
    {
      return;
    }
    bool aHigh = digitalRead(instance->_aPin);
    bool bHigh = digitalRead(instance->_bPin);
    portENTER_CRITICAL_ISR(&instance->_mux);
    if (aHigh == bHigh)
    {
      instance->_count--;
    }
    else
    {
      instance->_count++;
    }
    portEXIT_CRITICAL_ISR(&instance->_mux);
  }

  static SoftHalfQuadEncoder *instance;
  int _aPin = -1;
  int _bPin = -1;
  volatile int64_t _count = 0;
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};

SoftHalfQuadEncoder *SoftHalfQuadEncoder::instance = nullptr;

#endif
