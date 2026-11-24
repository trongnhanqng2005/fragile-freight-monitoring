# System Overview

## Implemented local device path

```text
MPU6050
  -> ESP32
     -> acceleration magnitude
     -> local impact classification (SAFE / WARNING / DANGER)
     -> status LEDs and buzzer
     -> MQTT telemetry and DANGER alert publish (QoS 0)
```

The ESP32 reads the MPU6050 over I2C, calculates magnitude and deviation from 1g, classifies the current state, drives local indicators, and publishes telemetry and alert messages while connected. The current implementation is described in [Firmware Design](../firmware/firmware-design.md).

## Defined connectivity path

```text
ESP32
  -> MQTT
     -> Node-RED
        -> Adafruit IO
        -> Telegram
```

The shared MQTT broker role, topics, payload contract, and delivery semantics are defined in the [MQTT Integration Contract](../mqtt/integration-contract.md). Local broker setup and the verified host-only smoke test are in the [Local MQTT Broker guide](../mqtt/local-broker.md).

The contract and firmware implement telemetry/alert publishing and command subscription. Firmware-to-broker delivery of telemetry and the DANGER alert was verified in VS Code Wokwi against a local Mosquitto broker. The repository now includes a Node-RED flow for validation, a live dashboard, alert routing, optional Adafruit IO/Telegram integrations, and alarm enable/disable downlink. Configure credentials and run the documented end-to-end checks before claiming those external services are runtime-verified. Spring Boot is a separate MQTT consumer/API and is not a replacement for Node-RED.
