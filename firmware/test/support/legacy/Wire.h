#pragma once

class TwoWire {
 public:
  bool begin(int, int) { return true; }
};

extern TwoWire Wire;
