# System Overview

## Implemented local device path

```text
MPU6050
  -> ESP32
     -> acceleration magnitude
     -> local impact classification (SAFE / WARNING / DANGER)
     -> status LEDs and buzzer
```

The ESP32 reads the MPU6050 over I2C, calculates magnitude and deviation from 1g, classifies the current state, and drives local indicators. The current implementation is described in [Firmware Design](../firmware/firmware-design.md).

## Defined connectivity path

```text
ESP32
  -> MQTT
     -> Node-RED
        -> Adafruit IO
        -> Telegram
```

The shared MQTT broker role, topics, payload contract, and delivery semantics are defined in the [MQTT Integration Contract](../mqtt/integration-contract.md). Local broker setup and the verified host-only smoke test are in the [Local MQTT Broker guide](../mqtt/local-broker.md).

The contract is defined, but the ESP32 Wi-Fi/MQTT client, Spring Boot MQTT subscriber, Node-RED MQTT flow, Adafruit IO integration, and Telegram integration are not implemented. Node-RED remains the required integration path to Adafruit IO and Telegram. Spring Boot is an additional MQTT consumer/backend, not a replacement for Node-RED.
