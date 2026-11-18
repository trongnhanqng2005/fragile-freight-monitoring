# System Overview

## Local device path

```text
MPU6050
  -> ESP32
     -> acceleration magnitude
     -> local impact classification (SAFE / WARNING / DANGER)
     -> status LEDs and buzzer
     -> MQTT telemetry and DANGER alert publish (QoS 0, non-retained)
```

The ESP32 reads the MPU6050 over I2C, calculates magnitude and deviation from 1g, classifies the current state, drives local indicators, and publishes telemetry and alert messages while connected. The current implementation is described in [Firmware Design](../firmware/firmware-design.md).

## Connectivity path

```text
ESP32
  -> MQTT
     -> Node-RED
        -> Adafruit IO (gforce, status)
        -> Telegram (DANGER alerts)
     -> Spring Boot backend
        -> REST API
```

The shared MQTT broker role, topics, payload contract, and delivery semantics are defined in the [MQTT Integration Contract](../mqtt/integration-contract.md); this overview does not change that contract. Local broker setup and the verified host-only smoke test are in the [Local MQTT Broker guide](../mqtt/local-broker.md).

The firmware implements the publish side. The backend consumes both topics, keeps the latest telemetry and alert in memory, and serves `GET /api/sensor/latest`, `GET /api/sensor/esp32-01`, and `GET /api/sensor/esp32-01/alert`. Node-RED validates MQTT payloads, writes the Adafruit IO `gforce` and `status` feeds, and sends Telegram only for alerts whose `status` is `DANGER`, with 1000 ms de-duplication. Node-RED and the backend consume the same messages independently; Spring Boot is not a relay or a replacement for Node-RED.

On 2026-11-18 ICT, firmware → host loopback Mosquitto delivery passed in Wokwi for VS Code with SAFE, WARNING, and DANGER telemetry and one DANGER alert; details are in [Firmware Design](../firmware/firmware-design.md) and [Local Setup and Verification](../development/local-setup.md). Actual Adafruit IO feed updates, Telegram message arrival, and a full end-to-end run with all modules remain pending and are not claimed as passed.
