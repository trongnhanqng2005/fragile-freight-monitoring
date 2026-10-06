# Fragile Freight Monitoring

## Problem

Fragile freight can be damaged by excessive movement during transport. The project is intended to monitor movement and make relevant readings and alerts available through connected services.

## Current goal

Build a small end-to-end monitoring path:

**MPU6050 → ESP32 → MQTT → Node-RED → Adafruit IO + Telegram**

Spring Boot is an additional backend component for consuming MQTT data and exposing web/API functionality. It complements the required Node-RED flow; it does not replace it.

## Core functional scope

- Read motion data from an MPU6050 using ESP32 firmware.
- Send device data over MQTT.
- Use Node-RED to orchestrate the MQTT flow and integrations.
- Present data through Adafruit IO and send notifications through Telegram.
- Develop an additional Spring Boot backend for MQTT consumption and web/API functionality.

## Explicit non-goals

GPS, route tracking, fleet management, authentication, microservices, mobile apps, and other functionality not listed in the current scope are out of scope.

## Main technologies

- Device: ESP32, Arduino framework, PlatformIO, Wokwi simulation configuration.
- Messaging: MQTT (planned; not configured yet).
- Orchestration/integration: Node-RED (planned; not configured yet).
- Backend: Java 21, Spring Boot, Maven.
- Visualization and notifications: Adafruit IO and Telegram (planned; not configured yet).

## Current implementation status

- **Firmware:** PlatformIO project targets `esp32dev` with Arduino. `firmware/src/main.cpp` currently prints `Fragile Freight Monitoring` to Serial once per second. No MPU6050 or MQTT code is present.
- **Simulation:** `firmware/diagram.json` contains an ESP32 board only, with no connected peripherals. `firmware/wokwi.toml` points to the PlatformIO build output.
- **Backend:** Spring Boot application scaffold and a context-load test exist. MQTT consumption, application APIs, and web functionality are not implemented yet.
- **Integrations:** MQTT broker details, Node-RED flow, Adafruit IO setup, and Telegram integration are not configured yet.

See [Architecture](ARCHITECTURE.md), [Development](DEVELOPMENT.md), and [Conventions](CONVENTIONS.md).
