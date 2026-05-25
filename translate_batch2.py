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

r_wifi = [
    ("// Wi-Fi功能支持", "// Hỗ trợ chức năng Wi-Fi"),
    ("// 复用主存储", "// Tái sử dụng storage chính"),
]

r_motors = [
    ("// 马达控制", "// Điều khiển Motor"),
]

r_webrc = [
    ("// Web遥控器实现", "// Triển khai Web RC"),
    ("// 兜底：CR   → \\r", "// Dự phòng: CR -> \\r"),
    ("// ==================== 主循环函数 ====================", "// ==================== Hàm vòng lặp chính ===================="),
]

r_params = [
    ("// 参数存储在闪存", "// Tham số được lưu trữ trong Flash"),
    ("// 整型参数若读到 NaN（flash 损坏），回退到代码默认值并重写", "// Nếu tham số int đọc được là NaN (lỗi flash), dùng giá trị mặc định của code và ghi đè"),
]

r_battery = [
    ("// 电池电压检测", "// Phát hiện điện áp pin"),
]

r_board = [
    ("// 开发板引脚配置", "// Cấu hình chân (Pin) trên Board"),
]

r_imu = [
    ("// IMU传感器驱动", "// Driver cảm biến IMU"),
]

replace_in_file("wifi.ino", r_wifi)
replace_in_file("motors.ino", r_motors)
replace_in_file("web_rc.ino", r_webrc)
replace_in_file("parameters.ino", r_params)
replace_in_file("battery.ino", r_battery)
replace_in_file("board_config.h", r_board)
replace_in_file("imu.ino", r_imu)
print("Batch 2 done")
