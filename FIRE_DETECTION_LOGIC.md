# Logic phân loại trạng thái báo cháy

## 1. SAFE

**Logic cũ**

- Không phát hiện lửa.
- MQ-2 không kích hoạt.
- Nhiệt độ dưới `40°C`.

**Logic mới**

- Khi hoạt động bình thường: không có lửa, MQ-2 không kích hoạt và nhiệt độ dưới `40°C`.
- Sau `WARNING/DANGER`: chỉ trở lại `SAFE` khi không còn lửa, không còn tín hiệu MQ-2 và nhiệt độ `≤38°C` liên tục 5 giây.
- Khoảng chênh 2°C là hysteresis, giúp trạng thái không nhảy liên tục quanh ngưỡng 40°C.

## 2. WARNING

**Logic cũ**

- Chỉ xảy ra khi không có lửa, không có tín hiệu MQ-2 và nhiệt độ từ `40°C` đến dưới `45°C`.
- Flame hoặc MQ-2 kích hoạt sẽ chuyển thẳng sang `DANGER`.

**Logic mới**

- Chuyển sang `WARNING` ngay khi có ít nhất một dấu hiệu bất thường: Flame, MQ-2 hoặc nhiệt độ `≥40°C`.
- Một tín hiệu đơn lẻ mới xuất hiện được xem là cảnh báo sớm, chưa lập tức kết luận đã có cháy.

## 3. DANGER

**Logic cũ**

- Chỉ cần một trong các điều kiện sau: Flame kích hoạt, MQ-2 kích hoạt hoặc nhiệt độ `≥45°C`.
- Không kiểm tra thời gian duy trì và không kết hợp nhiều cảm biến.

**Logic mới**

Chuyển sang `DANGER` khi có ít nhất một điều kiện:

- Flame duy trì liên tục 2 giây.
- Ít nhất 2/3 tiêu chí cùng tồn tại trong 3 giây: Flame, MQ-2, nhiệt độ `≥45°C`.
- Nhiệt độ `≥58°C` liên tục 5 giây.
- Nhiệt độ đang `≥40°C`, tăng ít nhất `10°C/phút` và tình trạng tăng nhanh duy trì 2 giây.

Sau khi đã vào `DANGER`, trạng thái được giữ cho đến khi toàn bộ điều kiện an toàn được xác nhận liên tục 5 giây.

## Tài liệu tham khảo

- [NIST – Multiple Sensor, Multiple Criteria Alarms](https://www.nist.gov/publications/test-methodology-multiple-sensor-multiple-criteria-alarms-0): cơ sở cho việc kết hợp nhiều cảm biến nhằm tăng khả năng phát hiện và giảm báo giả.
- [NIST – Advanced Fire Detection Using Multi-Signature Alarm Algorithms](https://www.nist.gov/publications/advanced-fire-detection-using-multi-signature-alarm-algorithms): cơ sở cho thuật toán sử dụng nhiều dấu hiệu thay vì chỉ một tín hiệu.
- [Winsen – MQ-2 Manual](https://www.winsen-sensor.com/d/files/PDF/Semiconductor%20Gas%20Sensor/MQ-2%20%28Ver1.4%29%20-%20Manual.pdf): MQ-2 phản ứng với cả khói và nhiều khí dễ cháy, nên một tín hiệu MQ-2 chưa đủ khẳng định có cháy.
- [Honeywell – Series 200 Rate-of-Rise Heat Detector](https://buildings.honeywell.com/dk/en/products/by-category/sensors/heat-detectors/series-200-rate-of-rise-heat-detector): tham khảo ngưỡng nhiệt 58°C và tốc độ tăng nhiệt 10°C/phút.

> Các khoảng 2, 3 và 5 giây là tham số lọc tín hiệu do nhóm chọn cho prototype và cần được xác nhận bằng kiểm thử; chúng không phải ngưỡng thời gian bắt buộc của tiêu chuẩn báo cháy.
