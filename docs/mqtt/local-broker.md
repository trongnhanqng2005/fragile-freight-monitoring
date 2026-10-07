# Local MQTT Broker

## Broker and local policy

Eclipse Mosquitto provides the local development broker on TCP port `1883`. The verified Windows installation is Mosquitto `2.1.2` at `D:\Program Files\Mosquitto`, with its service running. Local/demo authentication is currently anonymous; this is not a production security policy. Do not expose an unauthenticated broker to an untrusted network.

No Mosquitto configuration file is tracked in the project. The currently verified broker listens only on loopback addresses `127.0.0.1` and `::1`, which is sufficient for host-only development and the CLI smoke test below.

## Broker host by client environment

| Client environment | Broker host | Port |
|---|---|---|
| Host-side `mosquitto_pub` / `mosquitto_sub` | `localhost` | `1883` |
| Spring Boot running on the broker PC | `localhost` | `1883` |
| Node-RED running on the broker PC | `localhost` | `1883` |
| ESP32 in Wokwi VS Code | `host.wokwi.internal` | `1883` |
| Future physical ESP32 | LAN address of the broker machine | `1883` |

`localhost` is for clients running on the broker PC; it is not a universal broker hostname. Wokwi and a physical ESP32 are not localhost processes. Their later use requires Mosquitto to accept the required non-loopback connection. Any machine-specific development listener configuration must remain local and untracked; no listener configuration for Wokwi or LAN access is defined here. Keep unauthenticated access limited to a trusted local/demo network.

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
