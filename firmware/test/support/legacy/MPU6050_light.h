#pragma once

#include "Arduino.h"
#include "Wire.h"

class MPU6050 {
 public:
  explicit MPU6050(TwoWire&) {}
  byte begin() { return 0; }
  void update() {}
  float getAccX() const { return testHardware::ax; }
  float getAccY() const { return testHardware::ay; }
  float getAccZ() const { return testHardware::az; }
};
