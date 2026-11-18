# Local MQTT Broker

## Broker and local policy

Eclipse Mosquitto provides the local development broker on TCP port `1883`. The verified Windows installation is Mosquitto `2.1.2` at `D:\Program Files\Mosquitto`, with its service running. Local/demo authentication is currently anonymous; this is not a production security policy. Do not expose an unauthenticated broker to an untrusted network.

No Mosquitto configuration file is tracked in the project. The currently verified broker listens only on loopback addresses `127.0.0.1` and `::1`. That loopback listener was sufficient for the verified Wokwi for VS Code firmware → broker check when the simulator and broker ran on the same PC; see the contract-topic subscription below. A physical ESP32 is not a loopback client and requires a listener on the broker machine's LAN address, limited to a trusted network. Any machine-specific listener configuration must stay local and untracked.

## Broker host by client environment

| Client environment | Broker host | Port |
|---|---|---|
| Host-side `mosquitto_pub` / `mosquitto_sub` | `localhost` | `1883` |
| Spring Boot running on the broker PC | `localhost` | `1883` |
| Node-RED running on the broker PC | `localhost` | `1883` |
| ESP32 in Wokwi for VS Code on the broker PC | `host.wokwi.internal` | `1883` |
| Physical ESP32 on the LAN | LAN address of the broker machine | `1883` |

`localhost` is for clients running on the broker PC; it is not a universal broker hostname. Wokwi on the same PC reaches the host loopback listener through `host.wokwi.internal`, so no separate LAN listener is needed for the verified GUI setup. A physical ESP32 instead needs the broker machine's LAN address and a trusted-LAN listener; that listener configuration is not tracked here. Keep unauthenticated access limited to a trusted local/demo network.

## Contract-topic subscription

With the broker running, subscribe to both integration topics from the host:

```powershell
& "D:\Program Files\Mosquitto\mosquitto_sub.exe" `
  -h localhost `
  -p 1883 `
  -t "fragile-freight/esp32-01/telemetry" `
  -t "fragile-freight/esp32-01/alert" `
  -v
```

On 2026-11-18 ICT this subscription received SAFE, WARNING, and DANGER telemetry plus one non-retained QoS 0 DANGER alert from the `esp32dev` firmware running in Wokwi for VS Code 4.1.0 Community. The firmware used `host.wokwi.internal:1883`; the broker used its existing loopback listener. This is firmware → broker → host-subscriber evidence for the topics in the [shared MQTT integration contract](integration-contract.md); it is not evidence of delivery to Adafruit IO or Telegram.

## Host-only CLI smoke test

With the broker running on the host, open two terminals. Subscribe first:

```powershell
& "D:\Program Files\Mosquitto\mosquitto_sub.exe" `
  -h localhost `
  -p 1883 `
  -t "fragile-freight/dev-test/pubsub-check" `
  -C 1 `
  -v
```

Then publish from the other terminal:

```powershell
& "D:\Program Files\Mosquitto\mosquitto_pub.exe" `
  -h localhost `
  -p 1883 `
  -t "fragile-freight/dev-test/pubsub-check" `
  -m "publisher-to-broker-to-subscriber"
```

Observed successful subscriber output:

```text
fragile-freight/dev-test/pubsub-check publisher-to-broker-to-subscriber
```

This confirms publisher → broker → subscriber on loopback. The `fragile-freight/dev-test/pubsub-check` topic is temporary and is not part of the production MQTT contract. See the [shared MQTT integration contract](integration-contract.md) for production topics and payloads.
