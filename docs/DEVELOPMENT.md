# Development

The repository keeps `backend/` and `firmware/` as separate modules.

## Backend

The backend uses Spring Boot, Java 21, and Maven. From `backend/`, use the checked-in Maven Wrapper:

```powershell
.\mvnw.cmd test
.\mvnw.cmd spring-boot:run
```

The current test is a Spring application context-load test. MQTT consumption and application web/API endpoints are not implemented yet.

## Firmware

The firmware uses ESP32, PlatformIO, and the Arduino framework. PlatformIO configuration targets the `esp32dev` board.

Build from `firmware/` with PlatformIO Core installed:

```text
pio run
```

The current `firmware/src/main.cpp` starts Serial at **115200 baud** and prints this line every second:

```text
Fragile Freight Monitoring
```

### Wokwi simulation

`firmware/wokwi.toml` points Wokwi to `.pio/build/esp32dev/firmware.bin` and `.pio/build/esp32dev/firmware.elf`; build the firmware first. Open the `firmware/` project in VS Code with the Wokwi extension installed and start its simulator. The current diagram simulates an ESP32 only; no MPU6050 is connected.

## MQTT and Node-RED

**Not configured yet.** The repository does not define an MQTT broker, credentials, topic names, message schema, or Node-RED flow. Set these up before documenting runnable connection or flow commands.
