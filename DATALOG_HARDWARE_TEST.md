# Kiểm tra Data Log khi có phần cứng

## 1. Chuẩn bị

- Kết nối ESP32 và toàn bộ cảm biến đúng sơ đồ mạch.
- Kiểm tra `secrets.h` có Wi-Fi và Blynk Auth Token đang hoạt động.
- Trong Blynk phải có ba Event: `fire_warning`, `fire_danger`, `system_safe`.
- Blynk Template đã được **Save And Apply**.

## 2. Nạp và theo dõi chương trình

1. Mở `FireDetectionWithBlynk.ino` bằng Arduino IDE.
2. Chọn đúng board ESP32 và cổng COM.
3. Nhấn **Verify**, sau đó **Upload**.
4. Mở Serial Monitor ở `115200 baud`.
5. Chờ ESP32 kết nối Wi-Fi và Blynk Cloud.

## 3. Tạo các trạng thái để kiểm tra

Thực hiện lần lượt:

1. **WARNING:** làm nhiệt độ nằm trong khoảng từ ngưỡng Warning đến dưới ngưỡng Danger.
2. **SAFE:** để nhiệt độ trở lại dưới ngưỡng Warning và không kích hoạt Flame/MQ-2.
3. **DANGER:** kích hoạt Flame, MQ-2 hoặc làm nhiệt độ đạt ngưỡng Danger.

Mỗi lần trạng thái thay đổi, Serial Monitor phải xuất hiện:

```text
Blynk event logged: Status=...
```

## 4. Kiểm tra trên Blynk

1. Mở **Devices → Fire Detection System → Timeline/Notifications**.
2. Kiểm tra có Event đúng thời điểm: `Fire Warning`, `System Safe`, `Fire Danger`.
3. Mở từng Event và kiểm tra có: nhiệt độ, Flame, Smoke/Gas và trạng thái cửa.
4. Kiểm tra push notification của Warning và Danger.
5. Mở biểu đồ History và kiểm tra dữ liệu V0, V1, V2, V6 được lưu.

## 5. Kết quả đạt

- Mỗi lần đổi trạng thái chỉ tạo một Event, không ghi lặp liên tục.
- Event có đúng dữ liệu cảm biến và cửa.
- Timeline, History và push notification hoạt động.

Nếu không có log, kiểm tra lần lượt: trạng thái thiết bị Online, Event Code trên Blynk, Auth Token, Wi-Fi và dòng thông báo trên Serial Monitor.
