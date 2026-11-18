# Fragile Freight Monitoring

Fragile-freight monitoring built around an **ESP32 and MPU6050**. The firmware classifies impacts and alerts locally, then publishes MQTT telemetry and DANGER alerts. A Spring Boot backend consumes both topics and serves the latest readings over REST; a Node-RED flow validates the same messages and forwards them to Adafruit IO and Telegram.

## Architecture

```text
MPU6050
  -> ESP32 firmware
     -> acceleration magnitude and SAFE / WARNING / DANGER classification
     -> green / yellow / red LEDs and buzzer
     -> MQTT telemetry and DANGER alert
        -> Spring Boot backend (MQTT consumer; in-memory latest telemetry/alert; REST API)
        -> Node-RED (MQTT validation; Adafruit IO gforce/status feeds; Telegram on DANGER)
```

The ESP32 samples the MPU6050, classifies the impact state from the acceleration vector magnitude, drives the local outputs, and publishes telemetry and DANGER alerts while MQTT is connected. Local sampling, LEDs, and the buzzer do not depend on Wi-Fi or MQTT.

The backend subscribes to `telemetry` and `alert`, keeps the latest telemetry and alert in memory, and exposes `GET /api/sensor/latest`, `GET /api/sensor/esp32-01`, and `GET /api/sensor/esp32-01/alert`. Node-RED validates incoming MQTT payloads, updates the Adafruit IO `gforce` and `status` feeds, and sends Telegram only for an alert with `status` equal to `DANGER`, de-duplicated over 1000 ms. The two consumers are independent: the backend is not a relay, and Node-RED does not depend on the backend to receive messages. Node-RED is the required integration path to Adafruit IO and Telegram.

Topics, payloads, cadence, and delivery semantics are defined in the [MQTT Integration Contract](docs/mqtt/integration-contract.md).

## Verification status

Verified on 2026-11-18 ICT:

- 42 firmware native tests passed and `esp32dev` built; the `firmware.bin` SHA256 is `1576a79f3835ff55108444439c521e1dba1b483a08655e70847649d87b1336cf`.
- Wokwi for VS Code 4.1.0 Community delivered firmware → host loopback Mosquitto (`127.0.0.1:1883`, firmware address `host.wokwi.internal`). The 16:18:09–16:18:51 ICT capture drove `accelZ` to 0.95, 1.55, 2.0, then 0.95 and observed SAFE (~0.95g), WARNING (~1.55g), DANGER (~2g), and SAFE again, with one non-retained QoS 0 DANGER alert. GUI Serial output and LEDs were observed.
- Wokwi CLI v0.28.1 passed the state cycle and the buzzer waveform (149.277 ms, 2003 Hz, 598 transitions). Its MQTT gateway connection was reset, so broker delivery is checked with the VS Code GUI.
- Integrated local checks passed: backend commit `e2694df` passed Maven tests (9/9) on Java 21; cold-start Mosquitto, malformed-payload handling, reconnect, and recovery of both exact QoS 0 subscriptions passed. Node 24 flow tests passed (6/6). Node-RED 5.0.10 with `node-red-contrib-telegrambot` 19.0.3 was exercised against Mosquitto, a local feed sink, and a loopback HTTP stub; retained/malformed messages and duplicate-alert handling passed.
- Simultaneous Wokwi VS Code → Mosquitto → backend REST / Node-RED local-output run passed on 2026-11-18 ICT. Observed states: SAFE 16:28:01.045 (~0.949951172g), WARNING 16:28:16.736 (~1.54998779g), DANGER alert 16:28:41.709 and telemetry 16:28:42.465 (~1.99993896g), then SAFE 16:29:06.662 (~0.949951172g). MQTT input capture contained 77 telemetry and one alert, all QoS 0 and non-retained; one mock HTTP `sendMessage` was sent, and both local feed outputs and REST matched. See the [demo checklist](docs/development/demo-checklist.md) for repeatable steps and capture details.

Audible buzzer sound is not verified; only the waveform is. Real Adafruit IO and Telegram delivery remains **pending credentials and destination configuration**. Reproduction steps and impact values are in [Local Setup and Verification](docs/development/local-setup.md).

## Module entry points

| Module | Responsibility | Entry point |
|---|---|---|
| Firmware | ESP32 sampling, classification, local outputs, and MQTT publishing | [Firmware design](docs/firmware/firmware-design.md) |
| Backend | MQTT consumer with in-memory telemetry/alert state and REST endpoints | [Backend README](backend/README.md) |
| Node-RED | MQTT validation, Adafruit IO feeds, and DANGER-gated Telegram | [Node-RED README](node-red/README.md) |

Run each module's tooling from its own directory. Shared setup, broker, and verification details are indexed in the [documentation index](docs/README.md).

## Local verification

A local Mosquitto broker can support host-only MQTT checks, firmware-to-broker delivery in Wokwi, and local backend or Node-RED development. See the [Local MQTT Broker guide](docs/mqtt/local-broker.md) and [Local Setup and Verification](docs/development/local-setup.md).

Use the [demo and verification checklist](docs/development/demo-checklist.md) for startup, the SAFE / WARNING / DANGER demo, local outputs, MQTT and REST checks, Adafruit IO and Telegram checks, reconnect behavior, and evidence capture. Leave an item pending until the expected result has actually been observed; an accepted connection or a passing unit test is not delivery evidence.
