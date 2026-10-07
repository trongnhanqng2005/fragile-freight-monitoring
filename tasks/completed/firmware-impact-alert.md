# Firmware Impact Alert

## Goal

Use MPU6050 acceleration magnitude to classify local impact status and provide immediate LED/buzzer feedback.

## Baseline

- ESP32 + MPU6050 integrated
- I2C: SDA GPIO21, SCL GPIO22
- Ax/Ay/Az readings available in g
- Acceleration magnitude / G-force calculated from all three axes
- Wokwi Serial Monitor routed through explicit UART wiring
- MPU6050 remains at its default ±2g accelerometer range

## Scope

- SAFE / WARNING / DANGER classification using deviation from 1g
- Green/yellow/red status LEDs and a short buzzer alert
- Approximately 10 ms sensor sampling
- Approximately 250 ms Serial reporting

## Out of Scope

- WiFi
- MQTT
- Node-RED
- Spring Boot integration
- Adafruit IO
- Telegram

## Technical Decisions

### Impact metric and thresholds

```text
deviationG = abs(magnitudeG - 1.0f)
```

- **SAFE:** `deviationG < 0.4g`
- **WARNING:** `0.4g <= deviationG < 0.9g`
- **DANGER:** `deviationG >= 0.9g`

These are demo/calibration values, not universal cargo-damage limits. Classification uses vector magnitude, not an individual axis.

### Timing

- Sensor update cadence: approximately 10 ms using `millis()` timing
- Serial reporting cadence: approximately 250 ms, independently of sensor sampling

### GPIO and outputs

- Green LED: GPIO25
- Yellow LED: GPIO26
- Red LED: GPIO27
- Buzzer: GPIO32, using a short 2 kHz tone

SAFE lights green; WARNING lights yellow; DANGER lights red and triggers one 150 ms tone when entering danger. The buzzer does not continuously retrigger while danger persists. It re-arms only after the state returns to SAFE and at least 1 second has elapsed since the previous alert.

## Files Changed for Implementation

- `firmware/src/main.cpp`
- `firmware/diagram.json`

## Implementation Record

1. Added three status LEDs, 220 Ω current-limiting resistors, and a Wokwi buzzer.
2. Configured output GPIOs while preserving I2C GPIO21/22 and UART/Serial Monitor wiring.
3. Separated approximately 10 ms sensor sampling from approximately 250 ms Serial output.
4. Calculated acceleration magnitude and deviation from 1g.
5. Classified SAFE/WARNING/DANGER and drove the corresponding LED.
6. Added a one-shot buzzer alert with SAFE-plus-cooldown re-arming.
7. Added magnitude, deviation, and state to Serial output.

## Verification Record

### Static verification

- PlatformIO build: **passed**
- `git diff --check`: **passed**
- `firmware/diagram.json` validation: **passed**

### Wokwi runtime verification — user-reported results

#### SAFE

```text
accel = (0, 0, 1)
```

**PASS:** SAFE classification and green LED behavior.

#### WARNING

```text
accel = (1.0, 0.6, 1.0)
```

**PASS:** WARNING classification and yellow LED behavior; buzzer is expected off in WARNING.

#### DANGER

```text
accel = (1.7, 0, 1.0)
```

**PASS:** DANGER classification and red LED behavior. Wokwi visually indicated buzzer activity during DANGER (**PASS** for simulator trigger indication). Audible sound was **NOT OBSERVED** in VS Code; Windows Volume Mixer showed VS Code at 100% on the Default output device. Treat this as a Wokwi/VS Code host-audio limitation, not as evidence of firmware failure. Audible output is not claimed as verified.

#### Return to SAFE

```text
accel = (0, 0, 1)
```

**PASS:** return to SAFE and green LED behavior. The user also confirmed the buzzer re-armed after SAFE plus cooldown and triggered on a subsequent DANGER event.

Overall reported results: SAFE **PASS**; WARNING **PASS**; DANGER classification **PASS**; green/yellow/red LED behavior **PASS**; buzzer trigger indicated visually by Wokwi **PASS**; audible host sound **NOT OBSERVED**; return to SAFE **PASS**; subsequent-event re-arm **PASS**.

## Documentation Record

Project documentation has been reorganized under `docs/` and the root README reports implementation progress. Runtime results are recorded above. Audible host output remains unverified and is documented as a Wokwi/VS Code host-audio limitation.

## Status

- **Implementation:** COMPLETE
- **Static verification:** COMPLETE
- **Runtime verification:** COMPLETE — reported state/LED, visual buzzer trigger, SAFE return, and subsequent-event re-arm passed; audible host sound not observed
- **Documentation:** COMPLETE
- **Review:** COMPLETE
- **Overall:** COMPLETED

Audible output was not observed in VS Code and is recorded as a host-audio limitation, not as verified audible output or a firmware failure.
