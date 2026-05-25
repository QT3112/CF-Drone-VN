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

replacements = [
    ("// 故障安全保护", "// Bảo vệ an toàn lỗi (Failsafe)"),
    ("// 当前机身是否处于倒置（Z轴cos < INVERTED_COS_THRESHOLD）", "// Thân máy bay đang bị lật ngược? (cos trục Z < INVERTED_COS_THRESHOLD)"),
    ("// RC丢失超时时间（秒），可通过参数 SF_RC_LOSS_TIME 配置", "// Thời gian timeout mất kết nối RC (giây), cấu hình qua SF_RC_LOSS_TIME"),
    ("// 下降至停机的时间（秒），可通过参数 SF_DESCEND_TIME 配置", "// Thời gian hạ cánh đến khi tắt máy (giây), cấu hình qua SF_DESCEND_TIME"),
    ("// Web遥控器失联阈值(ms)，必须大于心跳间隔2000ms", "// Ngưỡng mất kết nối WebRC (ms), phải lớn hơn khoảng thời gian heartbeat 2000ms"),
    ("// 倒置保护参数", "// Thông số bảo vệ lật ngược"),
    ("// cos(134°)，倾角超过134°视为倒置（留出陀螺漂移裕量）", "// cos(134°), góc nghiêng > 134° xem như lật ngược (đã bù trừ sai số gyro)"),
    ("// 持续倒置超过1.5秒触发停机", "// Giữ trạng thái lật ngược > 1.5s sẽ tắt máy"),
    ("// Web RC变量声明", "// Khai báo biến Web RC"),
    ("// WebRC独立负责其超时（webRCLossFailsafe）", "// WebRC tự xử lý timeout độc lập (webRCLossFailsafe)"),
    ("// control*已统一涳盖SBUS/MAVLink/WebRC输入，直接检查即可", "// control* đã bao gồm đầu vào SBUS/MAVLink/WebRC, có thể kiểm tra trực tiếp"),
    ("// Web遥控器丢失保护", "// Bảo vệ mất kết nối Web RC"),
    ("// 使用毫秒直接比较，避免整数除法引入的最大1秒误差", "// So sánh trực tiếp bằng ms, tránh sai số 1s khi chia số nguyên"),
    ("// 倒置保护：机体Z轴与世界Z轴夹角超过120°持续1.5秒则停机", "// Bảo vệ lật ngược: Góc giữa trục Z máy bay và trục Z thế giới > 120° trong 1.5s -> tắt máy"),
    ("// 取机体Z轴在世界系的Z分量：正立时≈+1，倒置时≈-1", "// Lấy trục Z của máy bay trên hệ tọa độ thế giới: Đứng thẳng ≈ +1, lật ngược ≈ -1"),
    ("// 电池电压保护", "// Bảo vệ điện áp pin"),
    ("// L1（3.4V）：未解锁禁止解锁（在 control.ino 处理），怠速时自动上锁", "// L1 (3.4V): Chưa mở khóa thì cấm mở khóa (xử lý ở control.ino), đang idle tự động khóa"),
    ("// L2（2.8V）：飞行中仅 LED 快闪告警", "// L2 (2.8V): Đang bay thì chỉ nhấp nháy LED cảnh báo nhanh"),
    ("// L3（2.6V）：飞行中自动降落（复用 descend()，速度由 SF_DESCEND_TIME 控制）", "// L3 (2.6V): Đang bay thì tự động hạ cánh (dùng descend(), tốc độ do SF_DESCEND_TIME quyết định)"),
    ("// 未接电池，忽略", "// Không kết nối pin, bỏ qua"),
    ("// L3 已触发后保持降落，直到上锁，避免阈值附近反复进出", "// L3 đã kích hoạt thì tiếp tục hạ cánh cho đến khi khóa, tránh lặp đi lặp lại ở ngưỡng cắt"),
    ("// 飞行中：仅 L3 触发强动作；L2 继续由 LED 告警", "// Đang bay: Chỉ L3 kích hoạt hành động mạnh; L2 tiếp tục cảnh báo bằng LED"),
    ("// 解锁怠速：L1 触发自动上锁", "// Mở khóa nhưng đang idle: L1 kích hoạt tự động khóa"),
]

replace_in_file("safety.ino", replacements)
print("Done translating safety.ino")
