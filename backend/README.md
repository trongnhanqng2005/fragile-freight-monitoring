# Backend

Run from the repository root with `backend\mvnw.cmd -f backend\pom.xml spring-boot:run`.
The REST API serves the most recent in-memory state at
`GET /api/sensor/latest` and `GET /api/sensor/{deviceId}`. The separate
`GET /api/sensor/{deviceId}/alert` endpoint returns the latest alert.
MQTT defaults to `tcp://localhost:1883`; override it with `mqtt.broker-url` and disable the
subscriber with `mqtt.enabled=false`.

## Mosquitto-to-REST smoke test

With Mosquitto (`mosquitto`, `mosquitto_pub`, and `mosquitto_sub`) and `curl`
available, start the backend while the broker is stopped. Start Mosquitto on
port 1883, publish the contract telemetry, and check the REST response:

```sh
mosquitto -p 1883
mosquitto_pub -t fragile-freight/esp32-01/telemetry -m '{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1.00,"gForce":1.00,"status":"SAFE"}'
curl http://localhost:8080/api/sensor/latest
```

Stop and restart Mosquitto, publish telemetry again, and confirm the REST
response updates. MQTT delivery uses clean sessions, QoS 0, and non-retained
messages; the in-memory state is not persisted across backend restarts.
