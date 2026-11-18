#include <Arduino.h>
#include <MPU6050_light.h>
#include <Wire.h>
#include <ImpactLogic.h>
#include "NetworkAdapter.h"
#include <TelemetryTiming.h>

constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr int GREEN_LED_PIN = 25;
constexpr int YELLOW_LED_PIN = 26;
constexpr int RED_LED_PIN = 27;
constexpr int BUZZER_PIN = 32;
constexpr float GRAVITY_MS2 = 9.80665f;
constexpr unsigned long SAMPLE_INTERVAL_MS = 10;
constexpr unsigned long SERIAL_INTERVAL_MS = 250;
constexpr unsigned long BUZZER_DURATION_MS = 150;

using fragilefreight::ImpactLogic;
using fragilefreight::ImpactSnapshot;
using fragilefreight::ImpactState;
using fragilefreight::TelemetryTiming;
using fragilefreight::stateName;

MPU6050 mpu(Wire);
NetworkAdapter network;
ImpactLogic impactLogic;
TelemetryTiming telemetryTiming;
ImpactSnapshot latestSnapshot{};
bool hasValidSnapshot = false;
uint32_t lastSampleMs = 0;
uint32_t lastSerialMs = 0;
uint32_t buzzerStartedMs = 0;
bool buzzerActive = false;

void serviceTelemetry(uint32_t now) {
  if (telemetryTiming.due(now) && hasValidSnapshot) {
    network.publishTelemetry(latestSnapshot);
  }
}

void applyState(ImpactState state) {
  digitalWrite(GREEN_LED_PIN, state == ImpactState::SAFE ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, state == ImpactState::WARNING ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, state == ImpactState::DANGER ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  applyState(ImpactState::SAFE);
  noTone(BUZZER_PIN);

  Serial.println("Fragile Freight Monitoring boot");
  Serial.flush();

  const bool i2cInitialized = Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.print("I2C initialization: ");
  Serial.println(i2cInitialized ? "OK" : "FAILED");
  Serial.flush();
  if (!i2cInitialized) {
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Initializing MPU6050...");
  Serial.flush();
  const byte status = mpu.begin();
  Serial.print("MPU6050 initialization status: ");
  Serial.println(status);
  Serial.flush();
  if (status != 0) {
    Serial.print("MPU6050 initialization failed (status: ");
    Serial.print(status);
    Serial.println(")");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("MPU6050 initialized");

  network.begin();
}

void loop() {
  const uint32_t now = millis();

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    mpu.update();

    const float axG = mpu.getAccX();
    const float ayG = mpu.getAccY();
    const float azG = mpu.getAccZ();
    const ImpactState previousState = impactLogic.state();
    const ImpactSnapshot sample = impactLogic.update(axG, ayG, azG, now);
    if (sample.valid) {
      latestSnapshot = sample;
      hasValidSnapshot = true;
    }

    if (sample.state != previousState) {
      applyState(sample.state);
    }

    if (sample.dangerEvent) {
      tone(BUZZER_PIN, 2000);
      buzzerStartedMs = now;
      buzzerActive = true;
      network.publishAlert(sample);
    }

    if (now - lastSerialMs >= SERIAL_INTERVAL_MS) {
      lastSerialMs = now;
      Serial.print("Accel [m/s^2]: Ax=");
      Serial.print(sample.axG * GRAVITY_MS2, 2);
      Serial.print(" Ay=");
      Serial.print(sample.ayG * GRAVITY_MS2, 2);
      Serial.print(" Az=");
      Serial.print(sample.azG * GRAVITY_MS2, 2);
      Serial.print(" | Magnitude=");
      Serial.print(sample.magnitudeG, 2);
      Serial.print(" g | Deviation=");
      Serial.print(sample.deviationG, 2);
      Serial.print(" g | State=");
      Serial.println(stateName(sample.state));
    }
  }

  if (buzzerActive && now - buzzerStartedMs >= BUZZER_DURATION_MS) {
    noTone(BUZZER_PIN);
    buzzerActive = false;
  }

  network.update(now);
  serviceTelemetry(now);
}
