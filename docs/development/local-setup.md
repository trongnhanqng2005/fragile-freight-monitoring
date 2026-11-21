# Local Setup and Verification

The repository has separate backend, firmware, and Node-RED modules. Run each tool from its module directory. Use the [demo and verification checklist](demo-checklist.md) to record startup, demo, connectivity, reconnect, and evidence results.

## Backend

The backend uses Java 21, Spring Boot, and the checked-in Maven Wrapper. From `backend/` in PowerShell:

```powershell
.\mvnw.cmd test
.\mvnw.cmd spring-boot:run
```

The backend subscribes to the telemetry and alert topics, keeps the latest telemetry and alert in memory, and exposes `GET /api/sensor/latest`, `GET /api/sensor/esp32-01`, and `GET /api/sensor/esp32-01/alert`. See the [Backend README](../../backend/README.md) for response shapes and configuration.

## Node-RED

The Node-RED flow consumes the same two MQTT topics, validates the payloads, writes the Adafruit IO `gforce` and `status` feeds, and sends Telegram only for an alert whose `status` is `DANGER`, de-duplicated over 1000 ms. Setup and credentials are described in the [Node-RED README](../../node-red/README.md). Actual Adafruit IO feed updates and Telegram message arrival require configured cloud credentials and are not yet verified.

## Local MQTT development

The local Mosquitto broker and host-only CLI publisher-to-subscriber verification are documented in the [Local MQTT Broker guide](../mqtt/local-broker.md). Use the [MQTT Integration Contract](../mqtt/integration-contract.md) for shared topics, payloads, and delivery behavior. Broker hostnames depend on where each client runs; the guide distinguishes host-only clients from Wokwi and physical devices.

## Firmware tooling

The ESP32 project uses PlatformIO and Arduino. Local tooling belongs in the repository-root `.venv/` (ignored by Git), including PlatformIO Core and PySerial. If the local environment is not already present, create it from the repository root in PowerShell:

```powershell
py -m venv .venv
.\.venv\Scripts\python.exe -m pip install platformio pyserial
```

From `firmware/`, build the ESP32 environment explicitly and run the native test suite:

```powershell
..\.venv\Scripts\python.exe -m platformio run -e esp32dev
..\.venv\Scripts\python.exe -m platformio test -e native
```

On Windows, `firmware\scripts\pio.cmd` wraps the same project `.venv` and scopes PlatformIO's core and cache to ignored `.tools/`. From the repository root:

```powershell
.\firmware\scripts\pio.cmd run -e esp32dev
.\firmware\scripts\pio.cmd test -e native
```

`firmware/scripts/use_native_toolchain.py` selects the project-local MSYS2 UCRT64 GCC/G++ on Windows only; other hosts, including Ubuntu CI, use the system GCC. The workflow `.github/workflows/firmware.yml` runs `pio test -e native` and `pio run -e esp32dev` on `ubuntu-latest` for pull requests to `main` and pushes to `main`. CI does not start Wokwi, a broker, or a subscriber, so it does not verify runtime MQTT delivery.

The headless Wokwi CLI suite asserts local GPIO and Serial behavior, including when MQTT is unavailable. From the repository root:

```powershell
pwsh.exe -NoProfile -File .\firmware\scripts\run-wokwi-suite.ps1
```

That suite is a different layer from broker delivery: it does not require a host subscriber to receive anything.

The Wokwi PlatformIO extension is recommended in `firmware/.vscode/extensions.json`. The local `firmware/wokwi.toml` selects `.pio/build/esp32dev/firmware.bin` and `.pio/build/esp32dev/firmware.elf`; build first, then open the `firmware/` directory in VS Code and start the Wokwi simulation.

The diagram retains explicit ESP32 UART-to-Serial-Monitor wiring. Preserve the TX/RX connections for Serial output in Wokwi.

## Wokwi MQTT delivery check

Firmware-to-broker delivery uses the VS Code Wokwi simulator (verified with Wokwi for VS Code 4.1.0 Community) and a running local Mosquitto broker; the CLI suite and CI do not cover it. Start the broker as described in the [Local MQTT Broker guide](../mqtt/local-broker.md), then subscribe on the host to both contract topics:

```powershell
& "D:\Program Files\Mosquitto\mosquitto_sub.exe" `
  -h localhost `
  -p 1883 `
  -t "fragile-freight/esp32-01/telemetry" `
  -t "fragile-freight/esp32-01/alert" `
  -v
```

Build `esp32dev`, open `firmware/` in VS Code, start the Wokwi simulation, and move the acceleration controls as in the table below. The firmware connects to `host.wokwi.internal:1883`. When the VS Code Wokwi simulator runs on the same PC as the broker, the existing loopback listener on `127.0.0.1:1883` was sufficient; no additional LAN listener was required.

Verified on 2026-11-18 ICT in the 16:18:09–16:18:51 capture: the subscriber received telemetry for SAFE, WARNING, and DANGER and exactly one non-retained QoS 0 DANGER alert, and the GUI Serial output and LEDs were observed. Use the VS Code GUI for this check. Wokwi CLI v0.28.1 passed the state cycle and buzzer waveform but reported a gateway connection reset, so it is not used for MQTT delivery.

## Wokwi impact checks

Use the MPU6050 acceleration controls and observe Serial output, LEDs, and buzzer:

| Case | Set `accelZ` (g) | Expected |
|---|---|---|
| SAFE | `0.95` | ~0.95g, SAFE, green LED, buzzer off |
| WARNING | `1.55` | ~1.55g, ~0.55g deviation, WARNING, yellow LED, buzzer off |
| DANGER | `2.0` | ~2.0g, ~1.0g deviation, DANGER, red LED, one short buzzer tone |
| Return to SAFE | `0.95` | SAFE, green LED; buzzer re-arms after SAFE and cooldown |

These are the values used in the verified capture and are deterministic simulation checks, not calibrated physical safety limits. Record observed Wokwi results accurately. When audio is not observable, the buzzer waveform is acceptable evidence; do not claim audible output unless it was actually heard. This table covers device outputs only; firmware-to-broker delivery is checked separately by the subscriber procedure above. Actual Adafruit IO feed updates, Telegram message arrival, and a full end-to-end run remain pending.
