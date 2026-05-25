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

r_board = [
    ("// 板级硬件配置", "// Cấu hình phần cứng Board"),
    ("// 通过条件编译自动区分 ESP32、ESP32-C3 与 ESP32-S3，无需手动修改", "// Tự động phân biệt ESP32, ESP32-C3 và ESP32-S3 qua chỉ thị biên dịch có điều kiện, không cần sửa thủ công"),
    ("// ---- 电机引脚（数组顺序：RL, RR, FR, FL，与 motors.ino 中常量定义一致）----", "// ---- Chân Motor (Thứ tự mảng: RL, RR, FR, FL, giống hằng số trong motors.ino) ----"),
    ("// GPIO0/1/3/10 均无内置上拉，无 Strapping 功能（C3 的 Strapping 引脚仅 GPIO2/8/9）", "// GPIO0/1/3/10 đều không có pull-up nội, không có chức năng Strapping (Chân Strapping của C3 chỉ có GPIO2/8/9)"),
    ("// ---- 电池 ADC ----", "// ---- Chân ADC Pin ----"),
    ("// GPIO2 为 Strapping 引脚（内置弱上拉），但 ADC 输入模式下上拉电阻不影响测量精度", "// GPIO2 là chân Strapping (có pull-up yếu nội bộ), nhưng chế độ ADC input không bị ảnh hưởng độ chính xác"),
    ("// 启动后 Strapping 采样完成，analogRead() 调用时引脚为正常输入态", "// Sau khi khởi động, lấy mẫu Strapping hoàn tất, khi gọi analogRead() chân sẽ ở trạng thái input bình thường"),
    ("// ---- RC 串口（SBUS）----", "// ---- Serial RC (SBUS) ----"),
    ("// GPIO9 有板载 I2C SCL 上拉 + Boot 内置上拉，作为 SBUS RX 输入时 SBUS 空闲高电平与上拉一致", "// GPIO9 có pull-up SCL I2C trên board + pull-up Boot nội bộ, khi làm SBUS RX input mức cao idle của SBUS sẽ đồng nhất với pull-up"),
    ("// ---- IMU SPI（使用文档 / Arduino pins_arduino.h 默认引脚）----", "// ---- SPI IMU (Sử dụng chân mặc định trong tài liệu / Arduino pins_arduino.h) ----"),
    ("// 与 Arduino ESP32-C3 库 SPI 默认定义完全一致，SPI.begin() 可无参调用", "// Hoàn toàn giống định nghĩa SPI mặc định của thư viện Arduino ESP32-C3, có thể gọi SPI.begin() không tham số"),
    ("// 文档 SCK  = GPIO4", "// Tài liệu SCK  = GPIO4"),
    ("// 文档 MISO = GPIO5", "// Tài liệu MISO = GPIO5"),
    ("// 文档 MOSI = GPIO6", "// Tài liệu MOSI = GPIO6"),
    ("// 文档 SS   = GPIO7", "// Tài liệu SS   = GPIO7"),
    ("// ---- LED：无板载 LED，禁用 ----", "// ---- LED: Không có LED trên board, vô hiệu hóa ----"),
    ("// ---- 电机引脚：GPIO4-7，远离 FSPI 区（GPIO10-13），无 Strapping 约束 ----", "// ---- Chân Motor: GPIO4-7, xa khu vực FSPI (GPIO10-13), không bị ràng buộc Strapping ----")
]

replace_in_file("board_config.h", r_board)
print("Batch 4 (Final) done")
