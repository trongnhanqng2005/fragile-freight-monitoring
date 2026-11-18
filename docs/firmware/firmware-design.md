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

After sensor initialization, the firmware starts station-mode Wi-Fi (`Wokwi-GUEST`, channel 6) and connects to Mosquitto at `host.wokwi.internal:1883` using the regular `espMqttClient` 1.7.3 with its internal worker. Wi-Fi/MQTT startup and retries are managed from `loop()` with attempts spaced at least 5000 ms; callbacks only record status. Network unavailability does not gate the local sampling, LED, buzzer, or Serial paths.

While MQTT is connected, the firmware publishes QoS 0, non-retained messages:

- Telemetry, `fragile-freight/esp32-01/telemetry`, every 1000 ms using the latest valid sample. The JSON fields are exactly `deviceId`, `ax`, `ay`, `az`, `gForce`, and `status` (`SAFE`, `WARNING`, or `DANGER`).
- Alert, `fragile-freight/esp32-01/alert`, immediately on an armed DANGER event, once per event. The JSON fields are exactly `deviceId`, `status` (`DANGER`), and `gForce`.

Non-finite samples are not published; telemetry keeps using the latest valid sample. Offline or rejected alerts are dropped and are not queued or replayed after reconnect. There is no application-level queue, Last Will, or subscription.

## Runtime verification status

On 2026-11-18 ICT, 42 native behavior tests passed and the `esp32dev` environment built. The resulting `firmware.bin` SHA256 is `1576a79f3835ff55108444439c521e1dba1b483a08655e70847649d87b1336cf`.

Firmware-to-broker delivery passed in Wokwi for VS Code 4.1.0 (Community) against a host loopback Mosquitto at `127.0.0.1:1883`, with the firmware broker address `host.wokwi.internal`. In the 16:18:09–16:18:51 ICT capture, setting `accelZ` to 0.95, 1.55, 2.0, then 0.95 produced SAFE (~0.95g), WARNING (~1.55g), DANGER (~2g), and SAFE again. A host subscriber received telemetry in all three states and exactly one non-retained QoS 0 DANGER alert; Serial output and the LEDs were observed in the GUI.

The Wokwi CLI v0.28.1 state-cycle check passed, including the buzzer waveform: 149.277 ms at 2003 Hz (598 transitions). The CLI's MQTT gateway connection was reset, so broker delivery is checked with the VS Code GUI instead. A waveform is not evidence of audible sound; audibility remains unverified.

Adafruit IO feed writes, Telegram message arrival, and a full end-to-end run with the backend and Node-RED are pending. See the [Backend README](../../backend/README.md) and [Node-RED README](../../node-red/README.md) for module status.
