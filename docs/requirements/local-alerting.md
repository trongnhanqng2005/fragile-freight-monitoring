# Local Alerting Requirements

The ESP32 provides immediate local state feedback using three LEDs and a buzzer:

| State | LED behavior | Buzzer behavior |
|---|---|---|
| SAFE | Green on; yellow and red off | Off |
| WARNING | Yellow on; green and red off | Off |
| DANGER | Red on; green and yellow off | One short tone on entry |

The buzzer must not continuously retrigger while the state remains DANGER. Re-arm it only after the system returns to SAFE and the 1-second cooldown from the previous danger alert has elapsed. The alert is local; cloud or network notifications are out of scope.
