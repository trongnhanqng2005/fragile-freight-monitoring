#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

using byte = uint8_t;
constexpr int OUTPUT = 1;
constexpr int HIGH = 1;
constexpr int LOW = 0;

namespace testHardware {
inline uint32_t now = 0;
inline int leds[40] = {};
inline bool buzzerOn = false;
inline unsigned int buzzerFrequency = 0;
inline unsigned int toneCalls = 0;
inline unsigned int noToneCalls = 0;
inline float ax = 0.0f;
inline float ay = 0.0f;
inline float az = 1.0f;
inline std::string serialOutput;

inline void reset() {
  now = 0;
  for (int& led : leds) led = LOW;
  buzzerOn = false;
  buzzerFrequency = 0;
  toneCalls = 0;
  noToneCalls = 0;
  ax = 0.0f;
  ay = 0.0f;
  az = 1.0f;
  serialOutput.clear();
}
}

class TestSerial {
 public:
  void begin(unsigned long) {}
  void flush() {}
  void print(const char* value) { testHardware::serialOutput += value; }
  void print(char value) { testHardware::serialOutput += value; }
  template <typename T> void print(T value) {
    std::ostringstream output;
    output << value;
    testHardware::serialOutput += output.str();
  }
  template <typename T> void print(T value, int digits) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(digits) << value;
    testHardware::serialOutput += output.str();
  }
  void println() { testHardware::serialOutput += '\n'; }
  template <typename T> void println(T value) {
    print(value);
    println();
  }
};

extern TestSerial Serial;
uint32_t millis();
void pinMode(int pin, int mode);
void digitalWrite(int pin, int value);
void tone(int pin, unsigned int frequency);
void noTone(int pin);
void delay(unsigned long duration);
