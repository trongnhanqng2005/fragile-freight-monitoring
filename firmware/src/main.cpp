#include <Arduino.h>
#include <MPU6050_light.h>
#include <Wire.h>

#include <math.h>

constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr float GRAVITY_MS2 = 9.80665f;
constexpr unsigned long SAMPLE_INTERVAL_MS = 250;

MPU6050 mpu(Wire);

void setup() {
  Serial.begin(115200);
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
  mpu.update();

  const float axG = mpu.getAccX();
  const float ayG = mpu.getAccY();
  const float azG = mpu.getAccZ();
  const float magnitudeG = sqrtf(axG * axG + ayG * ayG + azG * azG);

  Serial.print("Accel [m/s^2]: Ax=");
  Serial.print(axG * GRAVITY_MS2, 2);
  Serial.print(" Ay=");
  Serial.print(ayG * GRAVITY_MS2, 2);
  Serial.print(" Az=");
  Serial.print(azG * GRAVITY_MS2, 2);
  Serial.print(" | Magnitude=");
  Serial.print(magnitudeG * GRAVITY_MS2, 2);
  Serial.print(" m/s^2 (");
  Serial.print(magnitudeG, 2);
  Serial.println(" g)");

  delay(SAMPLE_INTERVAL_MS);
}
