# Báo cáo kỹ thuật — Hệ thống giám sát va đập hàng hóa dễ vỡ

## 1. Mục tiêu

Thiết kế thiết bị IoT dùng ESP32 và MPU6050 để giám sát gia tốc kiện hàng, cảnh báo tại chỗ khi phát hiện va đập mạnh, gửi telemetry/cảnh báo qua MQTT và cho phép người vận hành bật/tắt còi cục bộ từ dashboard.

## 2. Kiến trúc

```text
MPU6050 → ESP32 → MQTT/Mosquitto → Node-RED → Dashboard
                                      ├── Adafruit IO (tùy chọn)
                                      └── Telegram Bot (tùy chọn)
Dashboard → MQTT command → ESP32 → Còi
```

- **Sensing:** MPU6050 đọc gia tốc ba trục qua I2C.
- **Network:** ESP32 gửi telemetry mỗi giây và cảnh báo khi vào DANGER; nhận lệnh MQTT.
- **Middleware:** Node-RED kiểm tra dữ liệu, phân loại trạng thái, xử lý rule và chuyển tiếp dữ liệu/cảnh báo.
- **Application:** Dashboard Node-RED hiển thị trạng thái/G-force, đồ thị lịch sử và nút bật/tắt còi. Adafruit IO và Telegram cần cấu hình tài khoản.

## 3. Phần cứng và thuật toán

ESP32 kết nối MPU6050 qua SDA GPIO21/SCL GPIO22, ba LED trạng thái ở GPIO25/26/27 và buzzer ở GPIO32. Firmware lấy mẫu khoảng 10 ms.

Với gia tốc theo đơn vị g:

```text
magnitudeG = sqrt(ax² + ay² + az²)
deviationG = abs(magnitudeG - 1.0)
```

- SAFE: `deviationG < 0.4g`
- WARNING: `0.4g ≤ deviationG < 0.9g`
- DANGER: `deviationG ≥ 0.9g`

Đây là ngưỡng demo, chưa phải giới hạn hư hỏng hàng hóa đã hiệu chuẩn. Buzzer phát một âm ngắn khi vào DANGER nếu được bật; LED và cảnh báo MQTT không bị ảnh hưởng bởi cài đặt buzzer.

## 4. Giao tiếp và rule

- Telemetry: `fragile-freight/esp32-01/telemetry` — JSON chứa `deviceId`, `ax`, `ay`, `az`, `gForce`, `status`.
- Cảnh báo: `fragile-freight/esp32-01/alert` — gửi khi phát sinh DANGER.
- Điều khiển: `fragile-freight/esp32-01/command` — `ALARM_ON` hoặc `ALARM_OFF`.
- Node-RED gửi Telegram khi có WARNING, DANGER, dữ liệu không hợp lệ hoặc trạng thái báo cáo không khớp ngưỡng cấu hình.
- Node-RED gửi G-force và trạng thái lên hai feed Adafruit IO sau khi cấu hình thông tin xác thực.

## 5. Kiểm thử và kết quả

Tài liệu dự án ghi nhận mô phỏng Wokwi cho SAFE, WARNING, DANGER, LED/buzzer, trở về SAFE và firmware-to-broker cho telemetry/cảnh báo. Các kịch bản này kiểm tra các phần riêng; cần lưu video/log khi thực hiện lại để nộp.

**Chưa có minh chứng runtime trong repo cho:** flow Node-RED đã deploy; Telegram/Adafruit IO bằng tài khoản thật; lệnh MQTT bật/tắt còi sau thay đổi downlink mới; demo end-to-end đủ bốn tầng. Không đánh dấu các mục này đã kiểm thử cho đến khi có log hoặc video thật.

## 6. Hạn chế và việc cần hoàn tất

- Hiệu chuẩn MPU6050/ngưỡng va đập theo loại hàng hóa và cách lắp đặt.
- Buzzer chỉ báo cục bộ; mất kết nối MQTT làm mất cảnh báo từ xa trong thời gian offline (không có hàng đợi).
- Cần cấu hình tài khoản Telegram/Adafruit IO và kiểm tra luồng thực tế.
- Cần lưu video/ảnh/log cho mô phỏng, dashboard và kiểm thử end-to-end; hoàn thiện slide/poster và rehearsal Q&A.

## 7. Hướng dẫn chạy

Xem [hướng dẫn thiết lập và kiểm chứng](development/local-setup.md), [hợp đồng MQTT](mqtt/integration-contract.md), và [README Node-RED](../node-red/README.md). Thay các placeholder thông tin tài khoản trước khi kiểm thử tích hợp bên ngoài.
