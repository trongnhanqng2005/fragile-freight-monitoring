# Module Integration / Node-RED

Module này nhận dữ liệu MQTT từ thiết bị, đẩy G-force và trạng thái lên Adafruit IO, và gửi cảnh báo DANGER qua Telegram. Topic và payload tuân theo contract chung của nhóm tại `docs/mqtt/integration-contract.md`. Module này không tự đặt thêm quy ước nào ngoài file đó.

## Luồng dữ liệu

ESP32 (hoặc script demo) → Mosquitto → Node-RED, từ đó chia hai nhánh:

- Telemetry (`fragile-freight/esp32-01/telemetry`): lấy `gForce` và `status` đẩy lên feed `gforce` và `status` của Adafruit IO. Node delay chỉ cho qua 1 tin mỗi 3 giây để không vượt giới hạn của gói miễn phí Adafruit, còn feed `status` chỉ nhận tin khi giá trị đổi.
- Alert (`fragile-freight/esp32-01/alert`): chỉ khi `status` là DANGER mới gửi tin Telegram gồm thiết bị và G-force. Có thêm cooldown 30 giây làm lớp bảo vệ thứ hai, dù firmware đã không lặp alert theo contract.

## Broker theo môi trường

| Client | Host | Port |
|---|---|---|
| Node-RED chạy trên máy có broker | `localhost` | 1883 |
| ESP32 trong Wokwi | `host.wokwi.internal` | 1883 |
| ESP32 thật | IP LAN của máy chạy broker | 1883 |

Để Wokwi hoặc ESP32 thật kết nối vào, broker phải nhận kết nối ngoài loopback. Theo `docs/mqtt/local-broker.md`, cấu hình listener là của riêng từng máy và không commit. Tạo file `node-red/mosquitto.conf` trên máy của bạn (đã được `node-red/.gitignore` chặn) với nội dung:

    listener 1883
    allow_anonymous true

Chỉ dùng trong mạng tin cậy khi dev, không đưa ra mạng công cộng.

## Cần cài trước

- Node.js (bản LTS) và Node-RED: `npm install -g node-red`
- Mosquitto (kèm `mosquitto_pub`, `mosquitto_sub`) và thêm vào PATH
- Node bổ sung cho Node-RED: `node-red-contrib-telegrambot` (cài qua Manage palette)
- Tài khoản Adafruit IO với hai feed `gforce` và `status`
- Một Telegram Bot do BotFather tạo, kèm chat ID của người nhận cảnh báo

## Chạy lại từ đầu

1. Chạy broker bằng file cấu hình local vừa tạo ở trên.(nếu service Mosquitto nền của Windows đang chiếm cổng 1883 thì Stop nó trong `services.msc` trước):
   `mosquitto -c node-red/mosquitto.conf -v`
2. Chạy Node-RED: `node-red`, rồi mở `http://127.0.0.1:1880`.
3. Import `node-red/flows.json` (menu ≡ → Import).
4. Nhập credential và sửa chat ID (xem phần dưới), sau đó Deploy.
5. Kiểm tra node nhận MQTT và hai node Adafruit hiện connected.

## Credential

Token và key không nằm trong repo. Mỗi người tự nhập vào Node-RED sau khi import:

- Node `Adafruit gforce` và `Adafruit status`: bấm bút chì cạnh ô Server, tab Security, điền Username Adafruit và Active Key vào ô Password.
- Node `Telegram sender`: bấm bút chì cạnh ô Bot, dán token từ BotFather.
- Node `gForce -> feed` và `status -> feed`: đổi phần username trong topic (`<username>/feeds/gforce`, `<username>/feeds/status`) thành username Adafruit của bạn.
- Node `Format DANGER`: sửa hằng số `CHAT_ID` thành chat ID Telegram của người nhận.

Không commit `.env`, file `*_cred.json` hay bất kỳ file nào chứa key.

## Chạy thử không cần thiết bị

Chạy kịch bản mô phỏng SAFE → WARNING → DANGER → SAFE theo đúng contract (alert chỉ gửi một lần, QoS 0):

    powershell -ExecutionPolicy Bypass -File node-red/demo.ps1

Hoặc gửi tay từng tin (PowerShell, JSON bọc nháy đơn và giữ `\"` bên trong):

    mosquitto_pub -h localhost -t "fragile-freight/esp32-01/telemetry" -m '{\"deviceId\":\"esp32-01\",\"ax\":0.02,\"ay\":-0.01,\"az\":1.0,\"gForce\":1.0,\"status\":\"SAFE\"}'

    mosquitto_pub -h localhost -t "fragile-freight/esp32-01/alert" -q 0 -m '{\"deviceId\":\"esp32-01\",\"status\":\"DANGER\",\"gForce\":1.97}'

## Demo checklist

| Bước | Việc làm | Kết quả đúng |
|---|---|---|
| 1 | Chạy broker và Node-RED, Deploy | Node nhận MQTT và hai node Adafruit hiện connected |
| 2 | Gửi telemetry SAFE | Tab Debug có mục Telemetry, Adafruit status = SAFE |
| 3 | Gửi telemetry WARNING | Biểu đồ G-force tăng, status = WARNING, không có tin Telegram |
| 4 | Gửi telemetry và alert DANGER | Status = DANGER, Telegram nhận đúng một tin |
| 5 | Gửi lại alert DANGER trong 30 giây | Telegram không có tin thứ hai |
| 6 | Gửi alert mang status SAFE | Telegram không có tin nào |

## Lỗi hay gặp

- `address already in use` khi chạy broker: service Mosquitto nền vẫn đang chạy, vào `services.msc` và Stop.
- Tin vào broker nhưng Node-RED không hiện gì: kiểm tra topic ở node nhận và node switch đúng từng ký tự, không thừa dấu cách ở đầu hay cuối.
- Node Adafruit báo đỏ: sai username hoặc Active Key trong tab Security.
- Adafruit không nhận dữ liệu: topic của node `gForce -> feed` phải đúng dạng `<username>/feeds/gforce`, và nhớ Deploy sau khi sửa.
- Telegram không có tin: node `Format DANGER` phải nối tới `Telegram sender`, và chat ID phải đúng.
- `mosquitto_pub` báo `Unknown option`: lỗi dấu nháy trên PowerShell, bọc JSON bằng nháy đơn và giữ `\"` bên trong.