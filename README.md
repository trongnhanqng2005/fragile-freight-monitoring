# Fragile Freight Monitoring

Local monitoring for fragile freight using an **MPU6050 and ESP32**. The device currently calculates acceleration magnitude, classifies local impact status, and drives status LEDs and a buzzer. MQTT integrations are planned; Spring Boot is an additional backend and does not replace Node-RED.

## Current architecture

Implemented: MPU6050 → ESP32 → magnitude/classification → LED/buzzer.

Planned: ESP32 → MQTT → Node-RED → Adafruit IO / Telegram. Spring Boot is a separate planned MQTT consumer/API component.

## Repository structure

```text
backend/       Spring Boot scaffold
firmware/      ESP32 PlatformIO firmware and Wokwi diagram
node-red/      Placeholder for the planned Node-RED flow
docs/          Requirements, hardware, architecture, firmware, and development docs
```

## Progress

- **Completed:** project setup; ESP32 + Wokwi + PlatformIO; MPU6050 integration; acceleration magnitude/G-force; Wokwi Serial Monitor routing; local impact-alert implementation.
- **Runtime verified in Wokwi:** SAFE/WARNING/DANGER classification and LEDs, visual buzzer activity, return to SAFE, and buzzer re-arm after cooldown. Audible buzzer sound was not observed in VS Code and is documented as a simulator/host-audio limitation.
- **Not implemented:** WiFi; MQTT; Node-RED flow; Spring Boot MQTT consumer/API; Adafruit IO; Telegram.

See the [documentation index](docs/README.md) for requirements, hardware, architecture, firmware design, and setup instructions.
