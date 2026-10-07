# ESP32 Hardware

## Selected board

The firmware targets the **Espressif ESP32 Dev Module**, identified in PlatformIO as `esp32dev`. It provides integrated WiFi for the planned connectivity phase, sufficient GPIO for the current LEDs and buzzer, and I2C support for the MPU6050. The board is supported by both PlatformIO and Wokwi.

The classic ESP32 development module is sufficient for the present sensor, local alert, and future WiFi/MQTT scope. ESP32-S3 or ESP32-C6 features are not required: the project has no current need for S3-specific USB/compute capabilities or C6 WiFi 6/802.15.4 capabilities.

## GPIO allocation

| Signal | ESP32 GPIO |
|---|---:|
| MPU6050 SDA | 21 |
| MPU6050 SCL | 22 |
| Green LED | 25 |
| Yellow LED | 26 |
| Red LED | 27 |
| Buzzer | 32 |

The Wokwi diagram connects each LED through a 220 Ω current-limiting resistor. The buzzer negative terminal and LED cathodes connect to ground.

## Wokwi Serial Monitor

The project diagram includes explicit UART wiring from `esp:TX` to `$serialMonitor:RX` and `esp:RX` to `$serialMonitor:TX`. Preserve these connections: they are the project-local workaround used to make Wokwi Serial output observable. Firmware Serial uses 115200 baud.
