# Node-RED middleware and dashboard

Import `flows.json` into Node-RED. The flow subscribes to the ESP32 telemetry and alert topics, validates incoming JSON, displays current values and a G-force chart, reports WARNING and DANGER events to Telegram, sends G-force and status to Adafruit IO, and publishes dashboard alarm controls to the ESP32 command topic.

## Install

Install Node-RED and the classic dashboard nodes, then start Node-RED with a local Mosquitto broker available at `localhost:1883`:

```powershell
npm install -g --unsafe-perm node-red
cd node-red
npm install
node-red
```

Open `http://localhost:1880`, import `flows.json`, deploy, and open `http://localhost:1880/ui`.

## Optional external services

The local dashboard and MQTT downlink work without external accounts. To use Telegram, set these environment variables before starting Node-RED:

```powershell
$env:TELEGRAM_BOT_TOKEN = "your-bot-token"
$env:TELEGRAM_CHAT_ID = "your-chat-id"
node-red
```

The flow sends a Telegram message for WARNING, DANGER, and malformed or implausible telemetry events. The bot token is read at runtime and is not stored in the flow. The flow logs a warning and skips the HTTP request if the variables are missing.

For Adafruit IO, create the `fragile-gforce` and `fragile-status` feeds, then edit the `Adafruit IO MQTT` broker configuration in Node-RED. It uses TLS on port 8883. Set the broker username to your Adafruit IO username and password to your AIO key. In the `Adafruit IO feed values` function node, replace `YOUR_AIO_USERNAME` in both topic strings with that username. The flow publishes G-force to `fragile-gforce` and status to `fragile-status`.

## Rules and commands

- `WARNING` telemetry sends a Telegram warning.
- A DANGER alert topic message sends a Telegram urgent alert.
- Telemetry whose reported state disagrees with the configured deviation thresholds is flagged as abnormal and sent to Telegram.
- Dashboard buttons publish `ALARM_ON` or `ALARM_OFF` to `fragile-freight/esp32-01/command`. This only enables/disables the local buzzer on future DANGER events; LEDs and remote alerts continue to work.

The broker and service credentials are intentionally not included. Never commit real bot tokens or Adafruit IO keys.
