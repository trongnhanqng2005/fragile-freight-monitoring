# System Overview

## Implemented local device path

```text
MPU6050
  -> ESP32
     -> acceleration magnitude
     -> local impact classification (SAFE / WARNING / DANGER)
     -> status LEDs and buzzer
```

The ESP32 reads the MPU6050 over I2C, calculates magnitude and deviation from 1g, classifies the current state, and drives local indicators. The current implementation is described in [Firmware Design](../firmware/firmware-design.md), and the completed implementation/verification record is in the [firmware impact alert task](../../tasks/completed/firmware-impact-alert.md).

## Planned connectivity path

```text
ESP32
  -> MQTT
     -> Node-RED
        -> Adafruit IO
        -> Telegram
```

This MQTT/Node-RED/integration path is planned and is not implemented or configured. Broker details, credentials, topics, and payload schema have not been defined.

Spring Boot is an additional planned MQTT consumer for backend/web/API functionality. It complements and does not replace the required Node-RED integration path. The current backend is a scaffold; MQTT consumption and application APIs are not implemented.
