# Fragile Freight Monitoring

Local monitoring for fragile freight using an **MPU6050 and ESP32**. The device calculates acceleration magnitude, classifies local impact status, and drives status LEDs and a buzzer. Firmware publishes telemetry and DANGER alerts over MQTT and accepts alarm enable/disable commands. Node-RED provides middleware, a dashboard, and optional Adafruit IO/Telegram integrations. Spring Boot is a separate MQTT consumer/API.

## Current architecture

Implemented: MPU6050 → ESP32 → magnitude/classification → LED/buzzer → MQTT.

Available in the repository: ESP32 → MQTT → Node-RED → dashboard; optional Adafruit IO / Telegram. Spring Boot is a separate MQTT consumer/API.

## Repository structure

```text
backend/       Spring Boot scaffold
firmware/      ESP32 PlatformIO firmware and Wokwi diagram
node-red/      Node-RED flow, dashboard, and integration setup
docs/          Requirements, hardware, architecture, firmware, and development docs
```

## Progress

- **Implemented:** project setup; ESP32 + Wokwi + PlatformIO; MPU6050 integration; acceleration magnitude/G-force; local impact alerts; MQTT telemetry, DANGER alerts, and alarm commands; Node-RED flow/dashboard; optional Adafruit IO and Telegram routing.
- **Runtime verified in Wokwi:** SAFE/WARNING/DANGER classification and LEDs, visual buzzer activity, return to SAFE, and buzzer re-arm after cooldown. Audible buzzer sound was not observed in VS Code and is documented as a simulator/host-audio limitation.
- **Needs user credentials/runtime setup:** Adafruit IO and Telegram external integrations. The Node-RED flow and setup instructions are provided, but these services need account credentials and an end-to-end demonstration.
- **Evidence still needed for final submission:** saved final test log/video, technical report, slides/poster, and live demo/Q&A record.

See the [documentation index](docs/README.md) for requirements, hardware, architecture, firmware design, and setup instructions.
