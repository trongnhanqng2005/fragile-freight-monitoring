# MQTT Integration Contract

## Purpose and scope

This is the shared MQTT interface for independent firmware, Spring Boot, and Node-RED development. It defines message topics, payloads, and delivery behavior; it does not implement any MQTT client or integration.

## Device identity and topics

The single demo device ID is `esp32-01` (lowercase ASCII, hyphen-separated). The device ID in the topic path must match the payload's `deviceId`.

| Message | Topic |
|---|---|
| Telemetry | `fragile-freight/esp32-01/telemetry` |
| Alert | `fragile-freight/esp32-01/alert` |

## Topic publishers and subscribers

The ESP32 firmware produces both application topics. Spring Boot and Node-RED consume them independently; Spring Boot is not a relay, and Node-RED does not depend on Spring Boot to receive MQTT messages.

| Topic | Publisher | Subscribers |
|---|---|---|
| `fragile-freight/esp32-01/telemetry` | Firmware / ESP32 | Spring Boot backend, Node-RED |
| `fragile-freight/esp32-01/alert` | Firmware / ESP32 | Spring Boot backend, Node-RED |

This separation lets each module develop and test against the broker and shared contract independently.

## Payload encoding

Messages are UTF-8 JSON objects. Numeric fields are finite JSON numbers, not quoted strings. No device timestamp is included.

### Telemetry

The telemetry object contains exactly the fields below. Publish the latest values in every state: `SAFE`, `WARNING`, and `DANGER`.

| Field | JSON type | Unit / allowed value | Meaning |
|---|---|---|---|
| `deviceId` | string | `esp32-01` | Device identity; must match the topic path. |
| `ax` | number | g | Acceleration on the X axis. |
| `ay` | number | g | Acceleration on the Y axis. |
| `az` | number | g | Acceleration on the Z axis. |
| `gForce` | number | g | Absolute acceleration vector magnitude. |
| `status` | string | `SAFE`, `WARNING`, or `DANGER` | Current impact state; exact uppercase values. |

Example:

```json
{
  "deviceId": "esp32-01",
  "ax": 0.02,
  "ay": -0.01,
  "az": 1.00,
  "gForce": 1.00,
  "status": "SAFE"
}
```

### Alert

The alert object contains exactly the fields below. An alert is a DANGER event, not a second periodic telemetry stream.

| Field | JSON type | Unit / allowed value | Meaning |
|---|---|---|---|
| `deviceId` | string | `esp32-01` | Device identity; must match the topic path. |
| `status` | string | Always `DANGER` | State that triggered the alert. |
| `gForce` | number | g | Absolute acceleration vector magnitude at the event. |

Example:

```json
{
  "deviceId": "esp32-01",
  "status": "DANGER",
  "gForce": 1.97
}
```

## Cadence and alert behavior

These cadences are independent:

| Activity | Cadence |
|---|---|
| Sensor sampling | Approximately 10 ms |
| Serial reporting | Approximately 250 ms |
| MQTT telemetry publishing | Every 1000 ms |

Publish telemetry in `SAFE`, `WARNING`, and `DANGER`. Publish an alert immediately on entering `DANGER` while alerting is armed; do not wait for the telemetry interval. Do not repeat alerts while DANGER persists. Re-arm only after the state returns to `SAFE` and the existing 1000 ms cooldown since the previous alert has elapsed. `WARNING` does not re-arm the alert.

## Delivery semantics

- QoS: 0.
- Retain: false (messages are not retained).
- Session: clean session/start; no offline queue.
- Timestamp: none; the ESP32 has no trusted wall-clock source.

Downstream Node-RED → Adafruit IO rate limiting is outside this contract.
