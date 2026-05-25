import sys

def replace_in_file(filepath, replacements):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Failed to read {filepath}: {e}")
        return
        
    for old, new in replacements:
        if old not in content:
            print(f"WARNING: String not found in {filepath}: {old}")
        content = content.replace(old, new)
        
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

r_led = [
    ("// 板载LED灯控制", "// Điều khiển đèn LED trên mạch"),
    ("// 慢闪：500ms 半周期 → 1 Hz", "// Nháy chậm: Chu kỳ 500ms -> 1 Hz"),
    ("// 快闪：62.5ms 半周期 → 8 Hz", "// Nháy nhanh: Chu kỳ 62.5ms -> 8 Hz"),
    ("// ---- 外部依赖声明 ----", "// ---- Khai báo phụ thuộc bên ngoài ----"),
    ("// 电池告警：按飞行状态选择阈值", "// Cảnh báo pin: Chọn ngưỡng theo trạng thái bay"),
    ("// 飞行中（thrustTarget >= 0.15）→ L2（2.8V），L1 在飞行中不适用", "// Đang bay (thrustTarget >= 0.15) -> L2 (2.8V), L1 không áp dụng khi bay"),
    ("// 未解锁 / 解锁怠速 → L1（3.4V）", "// Chưa mở khóa / Đang idle -> L1 (3.4V)"),
    ("// L2：飞行中", "// L2: Đang bay"),
    ("// L1：未解锁/怠速", "// L1: Chưa mở khóa / Đang idle"),
    ("// 检测是否有任意告警（低电 / 遥控失联 / 倒置）", "// Kiểm tra xem có cảnh báo nào không (Pin yếu / Mất RC / Lật ngược)"),
    ("// 倒置检测", "// Phát hiện lật ngược"),
    ("// 遥控失联检测（SBUS RC，仅解锁后）", "// Phát hiện mất kết nối RC (SBUS RC, chỉ sau khi mở khóa)"),
    ("// Web RC 失联检测：已激活但超时", "// Phát hiện mất kết nối Web RC: Đã kích hoạt nhưng timeout"),
    ("// 主循环调用：根据飞行状态驱动 LED", "// Gọi trong vòng lặp chính: Điều khiển LED dựa trên trạng thái bay"),
    ("// 解锁前低电：快闪", "// Pin yếu trước khi mở khóa: Nháy nhanh"),
    ("// 正常待机：常灭", "// Chờ bình thường: Tắt"),
    ("// 任何告警：快闪 8Hz", "// Bất kỳ cảnh báo nào: Nháy nhanh 8Hz"),
    ("// 正常飞行：慢闪 1Hz", "// Bay bình thường: Nháy chậm 1Hz"),
    ("// BOARD_LED_ENABLED == 0（ESP32-C3 无板载 LED）", "// BOARD_LED_ENABLED == 0 (ESP32-C3 không có LED trên mạch)"),
]

r_mavlink = [
    ("// MAVLink通讯", "// Giao tiếp MAVLink"),
]

r_cf = [
    ("// 主程序", "// Chương trình chính"),
    ("// 源代码使用Arduino IDE 2.3.x以上版本编译，不支持老版本的1.8.19！", "// Code sử dụng Arduino IDE bản 2.3.x trở lên, không hỗ trợ bản cũ 1.8.19!"),
    ("// 必须安装ESP32开发板核心库（即esp32 by Espressif Systems）和依赖（MAVLINK等）", "// Phải cài đặt core thư viện ESP32 (esp32 by Espressif Systems) và thư viện phụ thuộc (MAVLINK v.v.)"),
    ("// 开发板可选择ESP32 Dev Module或WeMOS D1 MINI ESP32", "// Board mạch có thể chọn ESP32 Dev Module hoặc WeMOS D1 MINI ESP32"),
    ("// 启用Web遥控器", "// Bật Web RC"),
    ("// 初始化Web遥控器", "// Khởi tạo Web RC"),
    ("// 读取Web遥控器输入", "// Đọc đầu vào Web RC"),
    ("// 将网页命令放到主循环执行，避免阻塞HTTP回调", "// Đưa lệnh Web vào vòng lặp chính để tránh block callback HTTP"),
]

r_quat = [
    ("// 轻量级旋转四元数库", "// Thư viện Quaternion quay nhẹ"),
]

r_pid = [
    ("// PID控制器实现", "// Triển khai bộ điều khiển PID"),
]

r_vector = [
    ("// 轻量级矢量库", "// Thư viện Vector nhẹ"),
]

r_time = [
    ("// 时间相关函数", "// Các hàm liên quan đến thời gian"),
]

r_log = [
    ("// RAM日志记录", "// Ghi log vào RAM"),
]

r_cli = [
    ("// 保持 HTTP 服务器在长时间命令（ca/cr）期间持续响应", "// Giữ server HTTP phản hồi trong quá trình chạy lệnh dài (ca/cr)"),
]

r_util = [
    ("// 实用工具函数", "// Các hàm tiện ích"),
]

r_control_h = [
    ("// 控制接口", "// Giao diện điều khiển"),
]

r_lpf = [
    ("// 低通滤波器", "// Bộ lọc thông thấp (Low Pass Filter)"),
]


replace_in_file("led.ino", r_led)
replace_in_file("mavlink.ino", r_mavlink)
replace_in_file("CF-Drone.ino", r_cf)
replace_in_file("quaternion.h", r_quat)
replace_in_file("pid.h", r_pid)
replace_in_file("vector.h", r_vector)
replace_in_file("time.ino", r_time)
replace_in_file("log.ino", r_log)
replace_in_file("cli.ino", r_cli)
replace_in_file("util.h", r_util)
replace_in_file("control.h", r_control_h)
replace_in_file("lpf.h", r_lpf)
print("Batch 1 done")
