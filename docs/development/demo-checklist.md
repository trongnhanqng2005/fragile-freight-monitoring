# Demo and Verification Checklist

Use this checklist to repeat the local demo and record each run's evidence. The 2026-11-18 ICT integrated local run and component checks passed as recorded below; use the unchecked boxes for a new run. Do not treat an accepted connection, startup log, or unit test alone as delivery evidence.

## Startup

- [ ] Start the local Mosquitto broker and a host subscriber for both contract topics as described in the [Local MQTT Broker guide](../mqtt/local-broker.md).
- [ ] Start the backend from `backend/` and confirm it starts cleanly; use the [Backend README](../../backend/README.md) for the run command and response shapes.
- [ ] Start Node-RED with the flow from `node-red/` and confirm its MQTT subscriptions connect; use the [Node-RED README](../../node-red/README.md) for setup.
- [ ] For a fresh broker start, verify backend and Node-RED reconnect and recover both exact MQTT contract subscriptions at QoS 0.
- [ ] Build and run the `esp32dev` firmware in Wokwi or on hardware; confirm Serial output reports I2C, MPU6050, Wi-Fi, and MQTT connection status.
- [ ] Confirm the backend REST endpoints respond: `GET /api/sensor/latest`, `GET /api/sensor/esp32-01`, and `GET /api/sensor/esp32-01/alert`. Record the request and response.

## SAFE / WARNING / DANGER demo

Drive the MPU6050 acceleration controls in g and observe the local outputs and telemetry:

| Case | Set `accelZ` (g) | Expected classification | Expected local output |
|---|---|---|---|
| SAFE | `0.95` | About 0.95g, deviation near 0.05g, `SAFE` | Green LED on; yellow and red off; buzzer off |
| WARNING | `1.55` | About 1.55g, about 0.55g deviation, `WARNING` | Yellow LED on; green and red off; buzzer off |
| DANGER | `2.0` | About 2.0g, about 1.0g deviation, `DANGER` | Red LED on; green and yellow off; one short buzzer tone |
| Return to SAFE | `0.95` | Back to `SAFE` | Green LED on; buzzer re-arms only after SAFE and the cooldown |

- [ ] SAFE classification and local outputs observed.
- [ ] WARNING classification and local outputs observed.
- [ ] DANGER classification and local outputs observed, including the single alert tone. When audio is not observable, a buzzer waveform is acceptable evidence; do not claim audible sound unless it was heard.
- [ ] Return to SAFE observed; the buzzer does not repeat while DANGER persists and re-arms after SAFE plus the cooldown.

These are demo/calibration values, not universal cargo-damage limits.

## Connectivity checks

- [ ] **MQTT:** the host subscriber receives telemetry for SAFE, WARNING, and DANGER, and receives an alert only on DANGER entry. Verify the topic names, `deviceId`, `status`, `gForce`, QoS 0, and non-retained delivery against the [MQTT Integration Contract](../mqtt/integration-contract.md).
- [ ] **REST:** each backend endpoint reflects the latest telemetry or alert it has consumed. Record the exact request and response.
- [ ] **Adafruit IO:** when credentials and feeds are configured, the `gforce` and `status` feeds receive the update from Node-RED. Record the observed feed value or screenshot.
- [ ] **Telegram:** when a bot and chat are configured, a Telegram message arrives for a `DANGER` alert only, without duplicates inside the 1000 ms window. Record the observed message or screenshot.

## Recorded integrated local pass (2026-11-18 ICT)

The following is a recorded result, not a pre-checked template for future runs:

- Backend commit `e2694df`: Maven / Java 21, 9/9 tests passed. Mosquitto cold start, malformed payload, reconnect, and recovery of both exact QoS 0 subscriptions passed.
- Node 24 flow tests: 6/6 passed. Real Node-RED 5.0.10 with actual `node-red-contrib-telegrambot` 19.0.3 used Mosquitto output to a local feed sink and its actual sender to a loopback HTTP stub. Retained and malformed messages and duplicate-alert tests passed.
- Simultaneous Wokwi VS Code → Mosquitto → REST / Node-RED local-output demo:

| ICT time | State/event | Observed acceleration |
|---|---|---|
| 16:28:01.045 | SAFE | 0.949951172g |
| 16:28:16.736 | WARNING | 1.54998779g |
| 16:28:41.709 | DANGER alert | 1.99993896g |
| 16:28:42.465 | DANGER telemetry | 1.99993896g |
| 16:29:06.662 | SAFE | 0.949951172g |

Captured MQTT input had 77 telemetry messages and one alert; all were QoS 0, non-retained. Exactly one mock HTTP `sendMessage` was sent. Both local feed outputs and backend REST matched. Local sinks/stubs must never be described as cloud delivery.

For repeatable runs, use the startup sequence and acceleration table above, subscribe to both contract topics, then compare the MQTT capture with backend REST and Node-RED local sink/stub outputs. Record actual versions, timestamps, counts, payload QoS/retain flags, and observed output values; preserve raw subscriber/serial capture and REST/stub evidence. Do not copy the recorded timestamps or pass results as evidence for a later run.

## Reconnect behavior

- [ ] Stop the broker or disconnect the network while the firmware is running; local sampling, classification, LEDs, and buzzer continue.
- [ ] Restart the broker or network; Wi-Fi and MQTT retries resume at their configured interval and telemetry publishing resumes after reconnection.
- [ ] DANGER alerts produced while offline are not queued or replayed; telemetry resumes with the current state after reconnect.
- [ ] Backend and Node-RED reconnect to the broker and resume consuming messages without requiring a firmware restart.

## Capture evidence

- [ ] Record the date, host, firmware environment, broker hostname, and module versions/commit used for the run.
- [ ] Save the Serial log, host subscriber output, and backend REST response or logs.
- [ ] Save replayable evidence for the buzzer: an audio recording if it was heard, otherwise the waveform with its duration, frequency, and transition count.
- [ ] Save the observed Adafruit IO and Telegram results when those services are part of the run.
- [ ] Capture MQTT input including both exact topics, QoS and retain flags; save backend REST responses and Node-RED local feed sink / HTTP stub results separately from any real cloud receipts.
- [ ] Save reconnect evidence showing the outage, retry/reconnection, and resumed telemetry.
- [ ] Keep credentials, tokens, and machine-specific broker configuration out of tracked files and evidence.

Record results in the relevant module documentation and here after verification. Leave unchecked items pending. Real Adafruit IO and Telegram delivery remains **pending credentials and destination configuration**.
