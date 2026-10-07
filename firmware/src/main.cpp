#include <Arduino.h>
#include <MPU6050_light.h>
#include <Wire.h>

#include <math.h>

constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr int GREEN_LED_PIN = 25;
constexpr int YELLOW_LED_PIN = 26;
constexpr int RED_LED_PIN = 27;
constexpr int BUZZER_PIN = 32;
constexpr float GRAVITY_MS2 = 9.80665f;
constexpr float WARNING_DEVIATION_G = 0.4f;
constexpr float DANGER_DEVIATION_G = 0.9f;
constexpr unsigned long SAMPLE_INTERVAL_MS = 10;
constexpr unsigned long SERIAL_INTERVAL_MS = 250;
constexpr unsigned long ALERT_COOLDOWN_MS = 1000;
constexpr unsigned long BUZZER_DURATION_MS = 150;

enum class ImpactState { SAFE, WARNING, DANGER };

MPU6050 mpu(Wire);
ImpactState impactState = ImpactState::SAFE;
unsigned long lastSampleMs = 0;
unsigned long lastSerialMs = 0;
unsigned long lastDangerAlertMs = 0;
unsigned long buzzerStartedMs = 0;
bool hasDangerAlert = false;
bool alertArmed = true;
bool buzzerActive = false;

ImpactState classifyImpact(float deviationG) {
  if (deviationG >= DANGER_DEVIATION_G) {
    return ImpactState::DANGER;
  }
  if (deviationG >= WARNING_DEVIATION_G) {
    return ImpactState::WARNING;
  }
  return ImpactState::SAFE;
}

const char* stateName(ImpactState state) {
  switch (state) {
    case ImpactState::WARNING:
      return "WARNING";
    case ImpactState::DANGER:
      return "DANGER";
    default:
      return "SAFE";
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
}

void loop() {
  const unsigned long now = millis();

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    mpu.update();

    const float axG = mpu.getAccX();
    const float ayG = mpu.getAccY();
    const float azG = mpu.getAccZ();
    const float magnitudeG = sqrtf(axG * axG + ayG * ayG + azG * azG);
    const float deviationG = fabsf(magnitudeG - 1.0f);
    const ImpactState newState = classifyImpact(deviationG);

    if (newState != impactState) {
      impactState = newState;
      applyState(impactState);
    }

    if (impactState == ImpactState::DANGER && alertArmed) {
      tone(BUZZER_PIN, 2000);
      buzzerStartedMs = now;
      lastDangerAlertMs = now;
      hasDangerAlert = true;
      alertArmed = false;
      buzzerActive = true;
    }

    if (impactState == ImpactState::SAFE && hasDangerAlert &&
        now - lastDangerAlertMs >= ALERT_COOLDOWN_MS) {
      alertArmed = true;
    }

    if (now - lastSerialMs >= SERIAL_INTERVAL_MS) {
      lastSerialMs = now;
      Serial.print("Accel [m/s^2]: Ax=");
      Serial.print(axG * GRAVITY_MS2, 2);
      Serial.print(" Ay=");
      Serial.print(ayG * GRAVITY_MS2, 2);
      Serial.print(" Az=");
      Serial.print(azG * GRAVITY_MS2, 2);
      Serial.print(" | Magnitude=");
      Serial.print(magnitudeG, 2);
      Serial.print(" g | Deviation=");
      Serial.print(deviationG, 2);
      Serial.print(" g | State=");
      Serial.println(stateName(impactState));
    }
  }

  if (buzzerActive && now - buzzerStartedMs >= BUZZER_DURATION_MS) {
    noTone(BUZZER_PIN);
    buzzerActive = false;
  }
}
