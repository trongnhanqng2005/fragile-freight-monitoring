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

## Connectivity and telemetry

After sensor initialization, the firmware starts station-mode Wi-Fi (`Wokwi-GUEST`, channel 6) and connects to Mosquitto at `host.wokwi.internal:1883` using the regular `espMqttClient` 1.7.3 with its internal worker. Wi-Fi/MQTT startup and retries are managed from `loop()` with attempts spaced at least 5000 ms; MQTT callbacks subscribe to the command topic and handle complete command messages. Network unavailability does not gate the local sampling, LED, buzzer, or Serial paths.

While MQTT is connected, the firmware publishes QoS 0, non-retained messages:

- Telemetry, `fragile-freight/esp32-01/telemetry`, every 1000 ms using the latest valid sample. The JSON fields are exactly `deviceId`, `ax`, `ay`, `az`, `gForce`, and `status` (`SAFE`, `WARNING`, or `DANGER`).
- Alert, `fragile-freight/esp32-01/alert`, immediately on an armed DANGER event, once per event. The JSON fields are exactly `deviceId`, `status` (`DANGER`), and `gForce`.
- Command subscription, `fragile-freight/esp32-01/command`, accepts `ALARM_ON` and `ALARM_OFF` to enable or disable the local buzzer for future DANGER events. This does not suppress LEDs or network alerts.

Non-finite samples are not published; telemetry keeps using the latest valid sample. Offline or rejected alerts are dropped and are not queued or replayed after reconnect. There is no application-level queue or Last Will. The only subscription is the command topic.

## Runtime verification status

The pre-downlink firmware baseline passed native behavior tests and the ESP32 build. Firmware-to-broker telemetry and alert delivery was verified in VS Code Wokwi with an activated license and a host subscriber. The new command subscription has not yet been built or exercised in Wokwi; verify it with the command procedure in [Local Setup and Verification](../development/local-setup.md) before claiming downlink runtime evidence.

## External service setup

Adafruit IO and Telegram need account credentials and an end-to-end runtime check. The Node-RED flow/dashboard and Spring Boot MQTT consumer/API are included in the repository.
