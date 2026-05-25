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
    ("// 飞行控制", "// Điều khiển bay"),
    ("// ============== 角速率环（内环）参数 ==============", "// ============== Thông số vòng lặp Rate (vòng trong) =============="),
    ("// 增大P值提高响应速度", "// Tăng P để phản hồi nhanh hơn"),
    ("// 中等I值补偿电机差异", "// Giá trị I trung bình để bù sai lệch motor"),
    ("// 小D值抑制震荡", "// Giá trị D nhỏ để giảm dao động"),
    ("// 限制积分积累", "// Giới hạn tích lũy I"),
    ("// 横滚和俯仰使用相同参数", "// Sử dụng cùng thông số cho Roll và Pitch"),
    ("// 偏航需要更高的P值（惯性较小）", "// Yaw cần giá trị P cao hơn (do quán tính nhỏ)"),
    ("// 中等I值补偿", "// Giá trị I trung bình để bù trừ"),
    ("// 小D值", "// Giá trị D nhỏ"),
    ("// ============== 角度环（外环）参数 ==============", "// ============== Thông số vòng lặp Angle (vòng ngoài) =============="),
    ("// 较高的P值快速响应", "// Giá trị P cao để phản hồi nhanh"),
    ("// 角度环通常不需要I项", "// Vòng lặp Angle thường không cần giá trị I"),
    ("// 角度环通常不需要D项", "// Vòng lặp Angle thường không cần giá trị D"),
    ("// 横滚和俯仰相同", "// Tương tự cho Roll và Pitch"),
    ("// 偏航响应稍慢", "// Phản hồi Yaw hơi chậm hơn"),
    ("// ============== 限制值 ==============", "// ============== Giá trị giới hạn =============="),
    ("// 高转速限制（1000°/s）", "// Giới hạn tốc độ quay tối đa (1000°/s)"),
    ("// 偏航转速稍低", "// Tốc độ quay Yaw chậm hơn một chút"),
    ("// 最大倾斜角30°", "// Góc nghiêng tối đa 30°"),
    ("// 定高悬停基础推力（stub，待气压计实现后替换）", "// Lực đẩy cơ bản khi lơ lửng ở chế độ Giữ độ cao (sẽ thay thế khi có cảm biến áp suất)"),
    ("// 解锁油门上限（归一化后 0~1），5%，超过此值禁止解锁", "// Giới hạn ga khi mở khóa (sau khi chuẩn hóa 0~1), 5%, vượt quá sẽ cấm mở khóa"),
    ("// 推力下限（摇杆最低位的输出），可通过参数 MOT_THR_MIN 调节", "// Giới hạn lực đẩy tối thiểu (ngõ ra khi cần ga ở mức thấp nhất), có thể chỉnh qua thông số MOT_THR_MIN"),
    ("// 推力上限（摇杆最高位的输出），可通过参数 MOT_THR_MAX 调节", "// Giới hạn lực đẩy tối đa (ngõ ra khi cần ga ở mức cao nhất), có thể chỉnh qua thông số MOT_THR_MAX"),
    ("// RC模式拨杆三挡对应的飞行模式，可通过参数 CTL_FLT_MODE_0/1/2 配置", "// Chế độ bay tương ứng với 3 mức công tắc trên RC, có thể cấu hình qua thông số CTL_FLT_MODE_0/1/2"),
    ("// SBUS手势解锁仅当WebRC未活跃时生效", "// Cử chỉ mở khóa SBUS chỉ có tác dụng khi WebRC không hoạt động"),
    ("// 防刷屏：低电禁止解锁提示", "// Chống spam log: Thông báo cấm mở khóa khi pin yếu"),
    ("// L1 及以下：禁止解锁，状态变化时提示", "// Mức L1 và thấp hơn: Cấm mở khóa, thông báo khi trạng thái thay đổi"),
    ("// 换新电池后重置", "// Reset sau khi thay pin mới"),
    ("// 底部死区 → 怠速，PID 不运行", "// Vùng chết đáy -> Chạy không tải (idle), PID không hoạt động"),
    ("// Web遥控器按钮处理（仅负责ARM/DISARM和模式切换）", "// Xử lý nút điều khiển trên WebRC (Chỉ phụ trách ARM/DISARM và chuyển chế độ)"),
    ("// 处理解锁/上锁按钮（上升沿检测，避免每个控制周期重复触发）", "// Xử lý nút mở khóa/khóa (Phát hiện sườn lên để tránh kích hoạt lặp lại mỗi chu kỳ)"),
    ("// 处理解锁/上锁状态变化日志", "// Log trạng thái thay đổi mở khóa/khóa"),
    ("// 按鈕。0：解锁（上升沿）", "// Nút 0: Mở khóa (Sườn lên)"),
    ("// 按钮1：上锁（上升沿）", "// Nút 1: Khóa (Sườn lên)"),
    ("// 按钮2：急停（上升沿）", "// Nút 2: Dừng khẩn cấp (Sườn lên)"),
    ("// 按钮6：STAB模式（上升沿）", "// Nút 6: Chế độ Cân bằng (STAB) (Sườn lên)"),
    ("// 按钮7：ACRO模式（上升沿）", "// Nút 7: Chế độ Nhào lộn (ACRO) (Sườn lên)"),
    ("// 按鈕。8：ALTHOLD定高模式（上升沿）- 暂未实现，跳过定高切到STAB，避免前端模式循环卡死", "// Nút 8: Chế độ Giữ độ cao (ALTHOLD) (Sườn lên) - Chưa hỗ trợ, chuyển về STAB để tránh lỗi tuần hoàn"),
    ("// 模式切换日志", "// Log thay đổi chế độ bay"),
]

replace_in_file("control.ino", replacements)
print("Done translating control.ino")
