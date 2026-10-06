# Fragile Freight Monitoring

Monitor movement of fragile freight using an **MPU6050 → ESP32 → MQTT → Node-RED → Adafruit IO / Telegram** flow. Spring Boot is an additional MQTT-consuming backend for web/API functionality; it does not replace Node-RED.

## Architecture

- **Device:** MPU6050 and ESP32 firmware
- **Messaging:** MQTT
- **Orchestration:** Node-RED
- **Backend:** Spring Boot
- **Visualization and notifications:** Adafruit IO and Telegram

## Repository structure

```text
backend/       Spring Boot module
firmware/      ESP32 PlatformIO and Wokwi module
node-red/      Planned Node-RED flow
docs/          Project, architecture, development, and convention docs
.opencode/     Project-local OpenCode skills/configuration
opencode.json  OpenCode project configuration
```

## Setup status

Backend and firmware scaffolds exist. Firmware currently prints a Serial message; the backend is a Spring Boot scaffold. MQTT, Node-RED, Adafruit IO, and Telegram are **not configured yet**. See [Development](docs/DEVELOPMENT.md) for current local build/simulation steps.

## Documentation

- [Project overview](docs/PROJECT.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Development](docs/DEVELOPMENT.md)
- [Conventions](docs/CONVENTIONS.md)
