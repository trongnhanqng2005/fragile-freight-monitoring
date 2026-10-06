# Architecture

## Component layers

| Layer | Component | Responsibility | Current status |
| --- | --- | --- | --- |
| Device/firmware | MPU6050 + ESP32 | Measure motion and prepare readings for publication. | Not implemented; the current firmware only writes a Serial message. |
| Messaging | MQTT | Transport device readings to consumers. | Not configured yet. |
| Orchestration/integration | Node-RED | Consume MQTT messages and coordinate the required integrations. | Not configured yet. |
| Backend | Spring Boot | Independently consume MQTT data and provide web/API functionality. It complements, and does not replace, Node-RED. | Scaffold only; MQTT and application endpoints are not implemented. |
| Visualization/notification | Adafruit IO + Telegram | Display selected readings and deliver notifications. | Not configured yet. |

## Main data flow

1. The MPU6050 provides motion measurements to the ESP32.
2. ESP32 firmware publishes readings to an MQTT broker.
3. Node-RED consumes the MQTT messages and routes the required data to Adafruit IO and Telegram.
4. Spring Boot is an additional MQTT consumer for backend/web/API use cases; it is not in place of the Node-RED route.

The broker, topics, payload format, thresholds, and integration details have not been defined in the current repository.

## Responsibilities

- **Firmware:** Read the sensor, perform only necessary device-side processing, and publish readings over MQTT. These responsibilities describe the target; sensor and MQTT handling are not implemented yet.
- **MQTT:** Provide message transport between the device and consumers. Broker and topic configuration are not configured yet.
- **Node-RED:** Remain the required orchestration layer for the MQTT-to-Adafruit-IO/Telegram flow. No flow is present yet.
- **Spring Boot:** Provide an additional backend that consumes MQTT data and exposes web/API functionality. Its current scaffold does not yet implement these behaviors.
- **Adafruit IO:** Provide the intended data visualization destination. No feed or connection is configured yet.
- **Telegram:** Provide the intended notification destination. No bot or notification flow is configured yet.
