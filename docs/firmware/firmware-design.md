# Firmware Design

## Current behavior

The PlatformIO `esp32dev` firmware initializes Serial at 115200 baud, starts I2C on SDA GPIO21/SCL GPIO22, and initializes the MPU6050 using the `MPU6050_light` library. The current default accelerometer range is ±2g per axis.

Approximately every 10 ms, the loop updates the sensor and reads Ax/Ay/Az in g. It calculates:

```text
magnitudeG = sqrt(axG * axG + ayG * ayG + azG * azG)
deviationG = abs(magnitudeG - 1.0)
```

Classification uses the complete vector magnitude:

- **SAFE:** deviation `< 0.4g`
- **WARNING:** deviation `>= 0.4g` and `< 0.9g`
- **DANGER:** deviation `>= 0.9g`

These are demo/calibration values, not universal cargo-damage limits.

## Local outputs

- SAFE: green LED on
- WARNING: yellow LED on
- DANGER: red LED on
- Entering DANGER: one 2 kHz, 150 ms tone on the buzzer

The alert is non-blocking. It does not repeat while danger persists. The buzzer re-arms once the state is SAFE and at least 1000 ms have passed since the previous alert.

## Serial reporting

Sensor sampling and Serial reporting use separate `millis()` timing. The firmware reports acceleration axes, magnitude in g, deviation from 1g, and state approximately every 250 ms.

## Not implemented

The firmware currently contains no WiFi or MQTT connectivity, Node-RED integration, backend client, or external notification integration.
