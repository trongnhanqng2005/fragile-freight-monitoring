# Local Setup and Verification

The repository has separate backend and firmware modules. Run each tool from its module directory.

## Backend

The backend uses Java 21, Spring Boot, and the checked-in Maven Wrapper. From `backend/` in PowerShell:

```powershell
.\mvnw.cmd test
.\mvnw.cmd spring-boot:run
```

The backend is currently a scaffold. MQTT consumption and application APIs are not implemented.

## Firmware tooling

The ESP32 project uses PlatformIO and Arduino. Local tooling belongs in the repository-root `.venv/` (ignored by Git), including PlatformIO Core and PySerial. If the local environment is not already present, create it from the repository root in PowerShell:

```powershell
py -m venv .venv
.\.venv\Scripts\python.exe -m pip install platformio pyserial
```

From `firmware/`, build with:

```powershell
..\.venv\Scripts\python.exe -m platformio run
```

The Wokwi PlatformIO extension is recommended in `firmware/.vscode/extensions.json`. The local `firmware/wokwi.toml` selects `.pio/build/esp32dev/firmware.bin` and `.pio/build/esp32dev/firmware.elf`; build first, then open the `firmware/` directory in VS Code and start the Wokwi simulation.

The diagram retains explicit ESP32 UART-to-Serial-Monitor wiring. Preserve the TX/RX connections for Serial output in Wokwi.

## Manual Wokwi impact checks

Use the MPU6050 acceleration controls (values in g) and observe Serial output, LEDs, and buzzer:

| Case | Set `(accelX, accelY, accelZ)` | Expected |
|---|---|---|
| SAFE | `(0, 0, 1)` | ~1.00g, SAFE, green LED, buzzer off |
| WARNING | `(1.0, 0.6, 1.0)` | ~1.54g, ~0.54g deviation, WARNING, yellow LED, buzzer off |
| DANGER | `(1.7, 0, 1.0)` | ~1.97g, ~0.97g deviation, DANGER, red LED, one short buzzer tone |
| Return to SAFE | `(0, 0, 1)` | SAFE, green LED; buzzer re-arms after SAFE and cooldown |

The values are deterministic simulation checks, not calibrated physical safety limits. Record observed Wokwi results accurately; do not claim audible output unless it was actually heard.
