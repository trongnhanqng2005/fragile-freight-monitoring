# Module Integration / Node-RED

Module này nhận dữ liệu MQTT từ thiết bị, đẩy G-force và trạng thái lên Adafruit IO, và gửi cảnh báo DANGER qua Telegram. Topic, payload và cadence tuân theo contract chung của nhóm tại [`docs/mqtt/integration-contract.md`](../docs/mqtt/integration-contract.md); module không tự đặt thêm quy ước nào ngoài file đó.

## Luồng dữ liệu

ESP32 (hoặc script demo) → Mosquitto → Node-RED, sau đó chia hai nhánh:

- Telemetry (`fragile-freight/esp32-01/telemetry`, firmware publish mỗi 1000 ms): lấy `gForce` và `status` đẩy lên feed `gforce` và `status` của Adafruit IO. Feed `gforce` được lấy mẫu tối đa 1 lần mỗi 3 giây; feed `status` chỉ nhận tin khi giá trị đổi. Đây là cách giảm tải cục bộ, không bảo đảm tuyệt đối quota của gói miễn phí Adafruit (quota còn phụ thuộc mức dùng chung và giới hạn thực tế của tài khoản), và không thay đổi cadence 1000 ms của thiết bị.
- Alert (`fragile-freight/esp32-01/alert`): chỉ alert DANGER hợp lệ mới đi tới Telegram; telemetry không bao giờ đi tới sender. `Validate contract` nhận payload thô và `JSON.parse` đúng một lần; JSON lỗi hoặc object có member trùng tên (kể cả tên viết bằng escape Unicode như `\u0073tatus`) bị bỏ trước khi kiểm tra schema và cooldown, vì `JSON.parse` gộp member trùng và có thể đổi nghĩa `status`. Bộ lọc yêu cầu đúng device, đúng kiểu, giá trị hữu hạn và đúng tập trường của contract (so khớp độ dài và own key). Alert trùng trong 1000 ms bị bỏ qua. Node MQTT in dùng MQTT 3.1.1 (`protocolVersion` = 4); giao thức này không hỗ trợ MQTT 5 Retain Handling (`rh`). Vì vậy flow bỏ mọi message có `msg.retain === true` trong `Validate contract`, tránh retained message được phát lại lúc khởi động đi tiếp.

Không có đường tắt nào từ telemetry tới Telegram: chỉ output alert của switch mới nối tới `Format DANGER` → `Telegram sender`.

## Broker

Flow mặc định dùng broker cục bộ qua loopback:

| Client | Host | Port |
|---|---|---|
| Node-RED chạy trên máy có broker | `localhost` | 1883 |
| `mosquitto_pub` / `mosquitto_sub` trên máy có broker | `localhost` | 1883 |
| ESP32 trong Wokwi VS Code | `host.wokwi.internal` | 1883 |
| ESP32 thật | IP LAN của máy chạy broker | 1883 |

`localhost` chỉ dùng cho tiến trình chạy trên chính máy có broker. Runtime đã kiểm tra: Wokwi VS Code 4.1.0 trên cùng máy kết nối thẳng tới Mosquitto đang lắng nghe loopback `127.0.0.1:1883` qua `host.wokwi.internal`, không cần listener LAN. ESP32 thật không phải tiến trình trên máy host nên cần listener trên mạng LAN tin cậy; cấu hình đó là của riêng từng máy, không được commit và phải giới hạn truy cập. Xem chính sách và cách kiểm tra loopback tại [`docs/mqtt/local-broker.md`](../docs/mqtt/local-broker.md); không mở broker ẩn danh ra mạng không tin cậy.

## Adafruit IO

Theo [tài liệu MQTT của Adafruit](https://io.adafruit.com/api/docs/mqtt.html), broker là `io.adafruit.com`, dùng cổng `8883` cho MQTT over TLS. Flow bật `Use TLS` và xác thực chứng chỉ máy chủ bằng CA hệ thống (tls-config `Adafruit system CAs`, `Verify server certificate` = true). Không tắt xác thực chứng chỉ và không dùng cổng 1883 cho Adafruit.

## Credential và biến môi trường

Token, key và chat ID không nằm trong repo. Sau khi import, mỗi người tự nhập:

- `Adafruit gforce` / `Adafruit status`: bút chì cạnh ô Server → tab Security, Username = username Adafruit, Password = Active Key.
- `Telegram sender`: bút chì cạnh ô Bot, dán token BotFather.
- Đặt biến môi trường `ADAFRUIT_USERNAME` = username Adafruit. Hai node `gForce -> feed` và `status -> feed` dựng topic `<ADAFRUIT_USERNAME>/feeds/gforce` và `<ADAFRUIT_USERNAME>/feeds/status`; nếu biến chưa đặt, node trả `null` nên không publish, tránh gửi nhầm feed.
- Đặt biến môi trường `TELEGRAM_CHAT_ID` = chat ID người nhận. `Format DANGER` đọc biến này bằng `env.get("TELEGRAM_CHAT_ID")`; nếu chưa đặt, không có tin nào tới sender.

Trên Windows PowerShell, đặt biến trong đúng cửa sổ trước khi khởi chạy Node-RED (cần restart Node-RED nếu thay đổi biến):

    $env:ADAFRUIT_USERNAME = "<username>"
    $env:TELEGRAM_CHAT_ID = "<chat-id>"
    node-red

Không commit `.env`, `flows_cred.json`, `settings.js` chứa secret, hay bất kỳ file nào chứa key/chat ID. Node-RED lưu credential đã nhập vào `flows_cred.json` (đã bị `.gitignore` chặn).

## Phụ thuộc

- Node.js 24 (test dùng `node:test`).
- Node-RED 5.0.10. Phần xử lý chỉ dùng core node: `mqtt in`, `function`, `switch`, `delay`, `rbe`, `debug`, `mqtt out`; không cần node-contrib nào cho MQTT hay xử lý dữ liệu.
- `node-red-contrib-telegrambot` 19.0.3 (đã ghim trong `flows.json`; cài qua Manage palette hoặc chạy `npm install node-red-contrib-telegrambot@19.0.3` trong thư mục người dùng Node-RED, mặc định `~/.node-red`) cho hai node Telegram.
- Mosquitto 2.x kèm `mosquitto_pub`, `mosquitto_sub` trong PATH.
- Tài khoản Adafruit IO với hai feed `gforce`, `status`; một Telegram bot do BotFather tạo.

Tương thích runtime: flow dùng MQTT 3.1.1 (`protocolVersion` = 4) cho broker input và Adafruit output. Retain Handling (`rh`) là tùy chọn MQTT 5 nên không tác dụng với kết nối MQTT 3.1.1; flow chủ động bỏ retained message trong function node. `mqtt out` có các trường MQTT 5 (`respTopic`, `contentType`, `userProps`, `correl`, `expiry`), nhưng Adafruit dùng MQTT 3.1.1 nên chúng chỉ là metadata và không được gửi.

## Chạy lại từ đầu

1. Chạy Mosquitto với cấu hình mặc định (chỉ loopback) theo [`docs/mqtt/local-broker.md`](../docs/mqtt/local-broker.md). Nếu service Mosquitto nền của Windows đang chiếm cổng 1883 thì Stop nó trong `services.msc` trước.
2. Chạy `node-red`, rồi mở `http://127.0.0.1:1880`.
3. Import `node-red/flows.json` (menu ≡ → Import).
4. Sau khi import, nhập credential trong UI; đặt `ADAFRUIT_USERNAME` và `TELEGRAM_CHAT_ID` trước khi khởi chạy Node-RED, sau đó Deploy.
5. Kiểm tra node nhận MQTT có tin và hai node Adafruit hiện connected qua TLS 8883.

## Kiểm thử cục bộ

Từ thư mục gốc repo:

    node --test node-red/test/flows.test.cjs

Các test đọc đồ thị flow và gọi function node trực tiếp; không kết nối broker, không gửi tin bên ngoài.

## Chạy thử không cần thiết bị

`demo.ps1` gửi SAFE → SAFE → WARNING → DANGER → SAFE theo đúng contract, QoS 0, không retain, cách nhau 4 giây; alert chỉ gửi ở bước DANGER:

    powershell -ExecutionPolicy Bypass -File node-red/demo.ps1

Mặc định script dùng `-Broker localhost` và `-Port 1883`; đổi bằng `-Broker <host> -Port <port>` nếu cần. Script đưa JSON qua stdin tới `mosquitto_pub` để tránh escape thủ công và dừng nếu publish lỗi.

Hoặc gửi tay từng tin (PowerShell, JSON bọc nháy đơn và giữ `\"` bên trong):

    mosquitto_pub -h localhost -t "fragile-freight/esp32-01/telemetry" -m '{\"deviceId\":\"esp32-01\",\"ax\":0.02,\"ay\":-0.01,\"az\":1.0,\"gForce\":1.0,\"status\":\"SAFE\"}'

    mosquitto_pub -h localhost -t "fragile-freight/esp32-01/alert" -q 0 -m '{\"deviceId\":\"esp32-01\",\"status\":\"DANGER\",\"gForce\":1.97}'

## Demo checklist

| Bước | Việc làm | Kết quả đúng |
|---|---|---|
| 1 | Chạy broker và Node-RED, Deploy | Node nhận MQTT có tin; hai node Adafruit connected qua TLS 8883 |
| 2 | Gửi telemetry SAFE | Tab Debug có mục Telemetry, Adafruit status = SAFE |
| 3 | Gửi telemetry WARNING | Biểu đồ G-force tăng, status = WARNING, không có tin Telegram |
| 4 | Gửi telemetry và alert DANGER | Status = DANGER, Telegram nhận đúng một tin |
| 5 | Gửi lại alert DANGER trong 1 giây | Telegram không có tin thứ hai (cooldown 1000 ms; firmware cũng không lặp alert khi DANGER còn tiếp diễn) |
| 6 | Gửi alert mang status SAFE | Telegram không có tin nào |

## Lỗi hay gặp

- `address already in use` khi chạy broker: service Mosquitto nền vẫn đang chạy, vào `services.msc` và Stop.
- Tin vào broker nhưng Node-RED không hiện gì: kiểm tra topic ở node nhận và node switch đúng từng ký tự, không thừa dấu cách ở đầu hay cuối.
- Node Adafruit báo đỏ: sai username hoặc Active Key trong tab Security, hoặc sai cổng/TLS. Nếu lỗi chứng chỉ, kiểm tra đồng hồ hệ thống và CA hệ thống; không tắt `Verify server certificate`.
- Adafruit không nhận dữ liệu: `ADAFRUIT_USERNAME` chưa đặt trong đúng tiến trình Node-RED (node function trả `null`), sai tên feed, hoặc chưa Deploy.
- Thấy dữ liệu cũ lặp lại: broker còn retained message từ lần publish trước; flow đã bỏ retained khi subscribe, nhưng có thể xoá retained trên broker.
- Telegram không có tin: node `Format DANGER` phải nối tới `Telegram sender`, `TELEGRAM_CHAT_ID` phải được đặt trong đúng tiến trình Node-RED, và bot token phải đúng.
- Script demo báo lỗi publish: kiểm tra broker/port và bảo đảm `mosquitto_pub` có trong PATH.
