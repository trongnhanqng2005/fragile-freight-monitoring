# Repository instructions

- This repository has separate modules, not a root build: `backend/` is Spring Boot (Java 21/Maven), `firmware/` is PlatformIO (ESP32/Arduino), and `node-red/` is only a placeholder. Node-RED remains part of the intended MQTT → Adafruit IO/Telegram flow; Spring Boot is an additional backend, not a replacement.
- Backend entrypoint: `backend/src/main/java/vn/edu/huit/fragilefreight/BackendApplication.java`. From `backend/`, run `./mvnw test` or `./mvnw spring-boot:run`; on Windows, use `mvnw.cmd` with the same arguments. The only current test checks application-context loading; MQTT consumption and API endpoints are not implemented.
- Firmware entrypoint: `firmware/src/main.cpp`; PlatformIO environment `esp32dev` uses Arduino. From `firmware/`, build with `pio run`. The current sketch only prints `Fragile Freight Monitoring` to Serial at 115200 baud once per second. Wokwi uses the `.pio/build/esp32dev/` output; build first.
- MQTT broker, topics, payload format, Node-RED flow, Adafruit IO, and Telegram are not configured. Do not invent configuration or credentials. Check `docs/` for the intended scope and current setup status.
- There is no root task runner, CI workflow, or configured Node-RED project. Verify changes using the affected module's available build/test command; do not assume a unified repository test command.
