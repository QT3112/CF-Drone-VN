import sys

def replace_in_file(filepath, replacements):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    for old, new in replacements:
        if old not in content:
            print(f"WARNING: String not found in {filepath}: {old}")
        content = content.replace(old, new)
        
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

replacements_rc = [
    ("// 使用RC接收器", "// Sử dụng bộ thu RC (Receiver)"),
    ("// SBUS RX 引脚，-1 表示禁用 RC，可通过参数 RC_RX_PIN 配置", "// Chân SBUS RX, -1 là vô hiệu hóa RC, có thể cấu hình qua tham số RC_RX_PIN"),
    ("// calibration zero values 每个通道的\"零点\"校准值（最小值/中心点）", "// calibration zero values Giá trị \"điểm 0\" của mỗi kênh (giá trị nhỏ nhất/trung tâm)"),
    ("// calibration max values 每个通道的\"最大值\"校准值", "// calibration max values Giá trị \"tối đa\" của mỗi kênh"),
    ("// Pin 未配置则跳过 RC 初始化", "// Bỏ qua khởi tạo RC nếu chân (Pin) chưa được cấu hình"),
    ("// RC 未启用", "// RC chưa được bật"),
    ("//第1步：初始化位置,操作：", "// Bước 1: Vị trí khởi tạo, Thao tác:"),
    ("// ✓ 所有摇杆归中位置", "// ✓ Tất cả cần đưa về giữa"),
    ("// ✓ 所有开关拨到默认位置（通常是锁定/中间位置）", "// ✓ Tất cả công tắc gạt về vị trí mặc định (thường là khóa/giữa)"),
    ("// ✓ 保持3秒不动", "// ✓ Giữ yên trong 3 giây"),
    ("//第2步：基础摇杆移动,操作：", "// Bước 2: Di chuyển cần cơ bản, Thao tác:"),
    ("// ✓ 左摇杆：向下", "// ✓ Cần trái: Kéo xuống"),
    ("// ✓ 右摇杆：居中", "// ✓ Cần phải: Giữa"),
    ("// ✓ 保持3秒", "// ✓ Giữ trong 3 giây"),
    ("//第3步：摇杆居中,操作：", "// Bước 3: Đưa cần về giữa, Thao tác:"),
    ("// ✓ 左摇杆：居中", "// ✓ Cần trái: Giữa"),
    ("//第4步：油门通道识别,操作：", "// Bước 4: Nhận diện kênh Ga, Thao tác:"),
    ("// ✓ 左摇杆：向上推到底（油门最大）", "// ✓ Cần trái: Đẩy lên cao nhất (Ga lớn nhất)"),
    ("//→ 系统自动识别油门通道", "// -> Hệ thống tự động nhận diện kênh Ga"),
    ("//第5步：偏航通道识别,操作：", "// Bước 5: Nhận diện kênh Xoay, Thao tác:"),
    ("// ✓ 左摇杆：向右推到底（偏航右转）", "// ✓ Cần trái: Đẩy hết sang phải (Xoay phải)"),
    ("// → 系统自动识别偏航通道", "// -> Hệ thống tự động nhận diện kênh Xoay"),
    ("//第6步：俯仰通道识别,操作：", "// Bước 6: Nhận diện kênh Tiến/Lùi, Thao tác:"),
    ("// ✓ 左摇杆：向下推到底", "// ✓ Cần trái: Kéo xuống đáy"),
    ("// ✓ 右摇杆：向上推到底（俯仰前进）", "// ✓ Cần phải: Đẩy lên cao nhất (Tiến tới)"),
    ("// → 系统自动识别俯仰通道", "// -> Hệ thống tự động nhận diện kênh Tiến/Lùi"),
    ("//第7步：横滚通道识别,操作：", "// Bước 7: Nhận diện kênh Nghiêng, Thao tác:"),
    ("// ✓ 右摇杆：向右推到底（横滚右转）", "// ✓ Cần phải: Đẩy hết sang phải (Nghiêng phải)"),
    ("// → 系统自动识别横滚通道", "// -> Hệ thống tự động nhận diện kênh Nghiêng"),
    ("//第8步：模式通道识别,操作：", "// Bước 8: Nhận diện kênh Chế độ bay, Thao tác:"),
    ("// ✓ 先将解锁开关拨回锁定位置", "// ✓ Trước tiên gạt công tắc mở khóa về vị trí khóa"),
    ("// ✓ 然后将模式开关拨到最高档位（如手动模式）", "// ✓ Sau đó gạt công tắc chế độ bay lên mức cao nhất (như chế độ thủ công)"),
    ("// → 系统自动识别模式通道", "// -> Hệ thống tự động nhận diện kênh Chế độ bay"),
]

replace_in_file("rc.ino", replacements_rc)

replacements_est = [
    ("// 姿态估计", "// Ước lượng tư thế bay"),
    ("// ============== 互补滤波器参数 ==============", "// ============== Thông số bộ lọc bù (Complementary Filter) =============="),
    ("// 陀螺仪权重", "// Trọng số của con quay hồi chuyển (Gyro)"),
    ("// 修正率", "// Tỷ lệ hiệu chỉnh"),
]
replace_in_file("estimate.ino", replacements_est)

print("Translated rc.ino and estimate.ino")
