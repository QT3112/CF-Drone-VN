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
    ("// ---- 电池 ADC：GPIO1（ADC1_CH0，S3 的 ADC1 范围为 GPIO1-10）----", "// ---- ADC Pin: GPIO1 (ADC1_CH0, dải ADC1 của S3 là GPIO1-10) ----"),
    ("// ---- RC 串口（SBUS）：GPIO8，与 FSPI/UART0 无冲突 ----", "// ---- Serial RC (SBUS): GPIO8, không xung đột với FSPI/UART0 ----"),
    ("// ---- IMU SPI：使用 ESP32-S3 FSPI 默认管脚 ----", "// ---- SPI IMU: Sử dụng các chân mặc định FSPI của ESP32-S3 ----"),
    ("// SCK=12（默认），MISO=13（默认），MOSI=11（默认），CS=10（默认 SS）", "// SCK=12 (mặc định), MISO=13 (mặc định), MOSI=11 (mặc định), CS=10 (mặc định SS)"),
    ("// ---- LED：新 PCB 接 GPIO2 普通 LED，驱动方式与 ESP32 相同 ----", "// ---- LED: PCB mới nối LED thường ở GPIO2, cách điều khiển giống ESP32 ----"),
    ("// ---- ESP32 默认配置 ----", "// ---- Cấu hình mặc định ESP32 ----"),
    ("// ---------------------- 默认ESP32 -------------------------", "// ---------------------- Mặc định ESP32 -------------------------")
]
r_battery = [
    ("// 电池电压采样引脚", "// Chân lấy mẫu điện áp pin"),
    ("// ADC多次采样取均值，越大噪声越低、响应越慢", "// Lấy trung bình nhiều mẫu ADC, giá trị càng lớn thì nhiễu càng thấp, phản hồi càng chậm"),
    ("// 分压比：上桥10kΩ+下桥33kΩ，公式 Vbat = Vadc × 43/33", "// Tỷ lệ chia áp: Điện trở trên 10kΩ + dưới 33kΩ, công thức Vbat = Vadc × 43/33"),
    ("// L1：空载/怠速预警门限（V）", "// L1: Ngưỡng cảnh báo không tải/idle (V)"),
    ("// L2：飞行中低电告警门限（V）", "// L2: Ngưỡng cảnh báo pin yếu khi đang bay (V)"),
    ("// L3：飞行中危急门限，自动降落（V）", "// L3: Ngưỡng nguy hiểm khi bay, tự động hạ cánh (V)"),
    ("// 低于此值视为未接电池，忽略所有告警", "// Thấp hơn giá trị này được xem là không lắp pin, bỏ qua mọi cảnh báo"),
    ("// 推力≥此值视为飞行中，L1不适用", "// Lực đẩy >= giá trị này xem như đang bay, L1 không áp dụng"),
    ("// 低压连续判定防抖时间（秒）", "// Thời gian debounce để xác nhận pin yếu liên tục (giây)"),
    ("// 全局最新电压，由 updateBatteryVoltage() 定期刷新", "// Điện áp mới nhất toàn cầu, được cập nhật định kỳ bởi updateBatteryVoltage()"),
    ("// 每 0.5 秒采样一次", "// Lấy mẫu mỗi 0.5 giây"),
]
r_lpf = [
    ("// Bộ lọc thông thấp (Low Pass Filter)实现", "// Triển khai Bộ lọc thông thấp (Low Pass Filter)")
]
r_control_h = [
    ("// 飞行模式定义", "// Định nghĩa chế độ bay"),
    ("// 气压计定高（油门=升降速率）", "// Giữ độ cao bằng áp kế (Ga = Tốc độ lên xuống)"),
    ("// 控制状态结构", "// Cấu trúc trạng thái điều khiển"),
    ("// 函数声明", "// Khai báo hàm"),
    ("// 辅助函数", "// Hàm phụ trợ"),
    ("// 外部函数声明", "// Khai báo hàm bên ngoài")
]
r_estimate = [
    ("// 基于陀螺仪和加速度计的姿态计算", "// Tính toán tư thế dựa trên con quay hồi chuyển (gyro) và gia tốc kế")
]
r_util = [
    ("//实用函数", "// Các hàm tiện ích"),
    ("// ESP32-C3 欠压复位阈值通过 menuconfig 配置，无需代码操作", "// Ngưỡng reset điện áp thấp của ESP32-C3 được cấu hình qua menuconfig, không cần chỉnh trong code")
]
r_imu = [
    ("// 陀螺仪加速度计相关设置", "// Các cài đặt liên quan đến con quay hồi chuyển và gia tốc kế"),
    ("// IMU 安装方向（欧拉角，单位 rad）。默认值 (0, 0, -PI/2) 对应本 PCB 的安装方式：", "// Hướng lắp đặt IMU (Góc Euler, rad). Mặc định (0, 0, -PI/2) tương ứng với cách lắp của PCB này:"),
    ("// 芯片正面朝上，X 丝印→飞行器右侧，Y 丝印→飞行器前方 → 转换公式 Vector(data.y, -data.x, data.z)", "// Mặt chip hướng lên, in lụa X -> bên phải máy bay, Y -> phía trước máy bay -> Công thức chuyển đổi Vector(data.y, -data.x, data.z)"),
    ("// 如需适配其他安装方向，修改此值并执行 `preset` 重置参数存储。", "// Nếu cần thích ứng với hướng lắp đặt khác, hãy sửa giá trị này và chạy lệnh `preset` để reset thông số đã lưu."),
    ("// 陀螺仪偏置低通估计滤波器", "// Bộ lọc thông thấp ước lượng độ lệch của Gyro")
]
r_motors = [
    ("// 使用MOSFET的电机输出控制", "// Điều khiển ngõ ra motor dùng MOSFET"),
    ("// 如果使用ESC，将pwmStop、pwmMin和pwmMax更改为适当的μs值，将pwmFrequency减小到400", "// Nếu dùng ESC, hãy đổi pwmStop, pwmMin và pwmMax sang giá trị us phù hợp, giảm pwmFrequency xuống 400"),
    ("// 电机引脚（对应 MOTOR_REAR_LEFT=0, MOTOR_REAR_RIGHT=1, MOTOR_FRONT_RIGHT=2, MOTOR_FRONT_LEFT=3）", "// Chân motor (Tương ứng MOTOR_REAR_LEFT=0, MOTOR_REAR_RIGHT=1, MOTOR_FRONT_RIGHT=2, MOTOR_FRONT_LEFT=3)"),
    ("// ESP32-C3 LEDC 默认 XTAL 时钟 40MHz，10-bit 上限 39062Hz；", "// Clock XTAL mặc định của ESP32-C3 LEDC là 40MHz, giới hạn 10-bit là 39062Hz;"),
    ("// ESP32 APB 80MHz，10-bit 上限 78125Hz；", "// ESP32 APB 80MHz, giới hạn 10-bit là 78125Hz;"),
    ("// 25kHz 在两者 XTAL/APB 下 prescaler 均可精确表示，安全余量充足", "// Ở tần số 25kHz, prescaler dưới cả XTAL/APB đều có thể biểu diễn chính xác, dư dả an toàn"),
    ("// -1 表示纯占空比模式（接 MOSFET 直驱）；接 ESC 时设为实际 PWM 最大值（μs）", "// -1 là chế độ Duty Cycle thuần túy (Nối MOSFET trực tiếp); Khi nối ESC thì đặt là giá trị PWM tối đa thực tế (us)"),
    ("// 先解绑所有引脚（重复调用时清理旧 LEDC 通道），再拉低防止误转", "// Giải phóng tất cả các chân trước (xóa kênh LEDC cũ khi gọi lặp lại), sau đó kéo xuống mức thấp để chống quay nhầm"),
    ("// 首次调用时无绑定会静默返回 false，无副作用", "// Lần đầu gọi nếu không có gán chân sẽ tự động trả về false, không có tác dụng phụ"),
    ("// 用 double 接收返回值，避免精度损失", "// Dùng double để nhận giá trị trả về, tránh mất độ chính xác"),
    ("// pwm 时间模式（接 ESC 用）", "// Chế độ thời gian PWM (Dùng cho nối ESC)"),
    ("// 纯占空比模式（接 MOSFET 直驱用）", "// Chế độ Duty Cycle thuần túy (Dùng cho nối MOSFET trực tiếp)")
]
r_webrc = [
    ("// 网页遥控服务端", "// Máy chủ Web RC"),
    ("// 飞控统一控制变量（供协议适配层写入，与 SBUS/MAVLink 共用）", "// Biến điều khiển FC hợp nhất (cho lớp điều hợp giao thức ghi, dùng chung với SBUS/MAVLink)"),
    ("// ==================== 配置常量 ====================", "// ==================== Hằng số cấu hình ===================="),
    ("// 连接超时：最后一次收包超过此时间(ms)视为断连；需大于心跳间隔", "// Timeout kết nối: Quá thời gian này (ms) tính từ gói cuối là mất kết nối; phải lớn hơn khoảng heartbeat"),
    ("// VBAT_ADC_PIN / VBAT_ADC_SAMPLES / VBAT_DIVIDER 已迁移至 battery.ino", "// VBAT_ADC_PIN / VBAT_ADC_SAMPLES / VBAT_DIVIDER đã được chuyển sang battery.ino"),
    ("// ==================== 连接状态标志 ====================", "// ==================== Cờ trạng thái kết nối ===================="),
    ("// Web RC 当前有有效连接（由 readWebRC() 每帧更新）", "// Web RC hiện có kết nối hợp lệ (cập nhật mỗi khung hình bởi readWebRC())"),
    ("// 当前正在使用 Web RC 控制（与 webRCEnabled 保持同步）", "// Hiện đang sử dụng điều khiển Web RC (Đồng bộ với webRCEnabled)"),
    ("// 收到过至少一次摇杆数据（首次连接前为 false）", "// Đã nhận được ít nhất 1 lần dữ liệu RC (false trước lần kết nối đầu)"),
    ("// Web 调试控制台开关：POST /console/enable 开启，开启后 print() 写入缓冲区", "// Công tắc Console debug Web: Bật qua POST /console/enable, khi bật print() sẽ ghi vào buffer"),
    ("// 待发送给前端的警告消息，发送一次后自动清空", "// Tin nhắn cảnh báo chờ gửi về frontend, gửi một lần rồi tự xóa"),
    ("// ==================== 摇杆暂存值（供状态端点读取）====================", "// ==================== Giá trị tạm thời của Cần (Cho endpoint trạng thái đọc)===================="),
    ("// 单位：油门 0~100（%），姿态轴 ±30（°），按下=1/松开=0", "// Đơn vị: Ga 0~100 (%), Trục nghiêng ±30 (°), Nhấn=1 / Thả=0"),
    ("// 16位按钮位掩码，bit0=解锁 bit1=上锁 bit2=急停 bit6=STAB bit7=ACRO bit8=ALTHOLD", "// Mask nút bấm 16-bit, bit0=mở_khóa bit1=khóa bit2=dừng_khẩn bit6=STAB bit7=ACRO bit8=ALTHOLD"),
    ("// 最后一次收包的 millis() 时间戳", "// Timestamp millis() của gói cuối nhận được"),
    ("// ==================== 灵敏度缩放 ====================", "// ==================== Độ nhạy (Scale) ===================="),
    ("// 最终输出 = 处理后角度 × scale / STICK_MAX，结果写入 control* ([-1,1])", "// Đầu ra cuối = Góc đã xử lý × scale / STICK_MAX, kết quả ghi vào control* ([-1,1])"),
    ("// 减小 scale → 飞机响应更柔和；增大 → 更灵敏", "// Giảm scale -> Máy bay phản ứng mượt hơn; Tăng -> Nhạy hơn"),
    ("// 油门最大功率限制，1.0=100%，0.8=最多只能推到80%油门", "// Giới hạn công suất tối đa của Ga, 1.0=100%, 0.8=Chỉ đẩy lên được 80% Ga"),
    ("// 横滚/俯仰灵敏度，建议范围 0.5~1.0", "// Độ nhạy Roll/Pitch, đề xuất 0.5~1.0"),
    ("// 偏航灵敏度，偏航惯性小故单独偏低，建议范围 0.3~0.8", "// Độ nhạy Yaw, do quán tính Yaw nhỏ nên để thấp, đề xuất 0.3~0.8"),
    ("// ==================== 死区 ====================", "// ==================== Vùng chết (Deadzone) ===================="),
    ("// 归一化空间（0~1 比例），摇杆归中误差在死区内输出恒为 0", "// Không gian chuẩn hóa (tỷ lệ 0~1), lỗi cần ở giữa nằm trong vùng chết sẽ output 0"),
    ("// 增大 → 中立区更宽、误触更少；减小 → 响应更灵敏但易漂移", "// Tăng -> Vùng giữa rộng hơn, ít chạm nhầm; Giảm -> Phản hồi nhạy hơn nhưng dễ trôi"),
    ("// 横滚/俯仰/偏航共用，6% 归一化死区", "// Roll/Pitch/Yaw dùng chung, vùng chết chuẩn hóa 6%"),
    ("// 油门低端死区，推杆低于此比例时输出 0%（防抖）", "// Vùng chết cực dưới của Ga, đẩy dưới mức này sẽ output 0% (Chống nhiễu)"),
    ("// ==================== 输入值域（勿随意修改，需与前端保持一致）====================", "// ==================== Khoảng giá trị đầu vào (Đừng tùy tiện sửa, cần đồng bộ với Frontend)===================="),
    ("// 油门输出下限（%）", "// Giới hạn dưới đầu ra Ga (%)"),
    ("// 油门输出上限（%）", "// Giới hạn trên đầu ra Ga (%)"),
    ("// 姿态轴最大角度（°），对应前端满偏；修改需同步调整 PID TILT_MAX", "// Góc trục tối đa (°), tương ứng kéo hết mức ở Frontend; Sửa cái này thì phải đổi PID TILT_MAX theo"),
    ("// 前端摇杆归一化满偏值，前端 JS 固定输出 ±100", "// Mức chuẩn hóa kéo hết của Frontend, JS ở Frontend fix output ±100"),
    ("// ==================== 内部状态（运行时，勿手动修改）====================", "// ==================== Trạng thái nội bộ (Khi chạy, ĐỪNG tự ý sửa)===================="),
    ("// 上次通过验证的油门值，异常时回退使用", "// Giá trị Ga đã xác thực lần trước, để dùng lại nếu có lỗi"),
    ("// 上次通过验证的横滚值", "// Giá trị Roll đã xác thực lần trước"),
    ("// 上次通过验证的俯仰值", "// Giá trị Pitch đã xác thực lần trước"),
    ("// 上次通过验证的偏航值", "// Giá trị Yaw đã xác thực lần trước"),
    ("// 上次数据异常时间，用于限速错误日志输出", "// Thời điểm bị lỗi data lần trước, dùng để limit log báo lỗi"),
    ("// ==================== Web 服务器 ====================", "// ==================== Server Web ===================="),
    ("// ==================== 控制台日志缓冲区 ====================", "// ==================== Buffer log Console ===================="),
    ("// 单调递增总行数，用于增量拉取", "// Tổng số dòng tăng đơn điệu, dùng để fetch lần lượt (increment pull)"),
    ("// 按 \\n 拆分写入，避免换行符污染 JSON", "// Split bằng \\n khi viết, tránh dấu ngắt dòng làm hỏng cấu trúc JSON"),
    ("// ==================== 摇杆处理 ====================", "// ==================== Xử lý tay gạt (RC) ===================="),
    ("// 归一化空间死区（输入/输出均为 [-1, 1]）", "// Vùng chết (Chuẩn hóa, input/output đều là [-1, 1])"),
    ("// 超出死区的部分线性重映射到满幅，满推摇杆=满输出", "// Phần ngoài vùng chết sẽ được map tuyến tính tới mức cao nhất, kéo hết tay = đẩy hết mức"),
    ("// 处理姿态轴（横滚/俯仰/偏航）", "// Xử lý trục cân bằng (Roll/Pitch/Yaw)"),
    ("// 输入 raw ∈ [-100, +100]，输出 ∈ [-STICK_MAX, +STICK_MAX] (°)", "// Input raw thuộc [-100, +100], output thuộc [-STICK_MAX, +STICK_MAX] (°)"),
    ("// 处理油门轴", "// Xử lý trục Ga"),
    ("// 前端：左摇杆Y轴，底部=-100，顶部=+100", "// Frontend: Trục Y trái, Đáy=-100, Đỉnh=+100"),
    ("// 输出：pct ∈ [0, 100]", "// Output: pct thuộc [0, 100]"),
    ("// 线性映射：-100→0%，0→50%，+100→100%", "// Mapping tuyến tính: -100->0%, 0->50%, +100->100%"),
    ("// ==================== 核心数据处理 ====================", "// ==================== Xử lý dữ liệu lõi ===================="),
    ("// 暂存处理后的值（供状态端点读取）", "// Lưu tạm giá trị đã xử lý (Cho endpoint trạng thái đọc)"),
    ("// 写入统一控制变量（与 SBUS/MAVLink 同路径）", "// Viết vào các biến điều khiển toàn cục (Chung với SBUS/MAVLink)"),
    ("// ==================== JSON 协议处理器 ====================", "// ==================== Trình xử lý giao thức JSON ===================="),
    ("// 摇杆数据", "// Dữ liệu Remote RC"),
    ("// 按钮事件", "// Sự kiện nút nhấn"),
    ("// 心跳", "// Gói Heartbeat"),
    ("// ==================== HTTP 请求处理 ====================", "// ==================== Xử lý HTTP Request ===================="),
    ("// 发送后立即清空", "// Gửi xong sẽ xóa rỗng ngay"),
    ("// ==================== 电池电压 ====================", "// ==================== Điện áp pin ===================="),
    ("// readBatteryVoltage() 已迁移至 battery.ino，此处直接调用即可", "// readBatteryVoltage() đã chuyển sang battery.ino, gọi trực tiếp là được"),
    ("// ==================== 连接状态 ====================", "// ==================== Trạng thái kết nối ===================="),
    ("// ==================== 主设置函数 ====================", "// ==================== Hàm khởi tạo chính (setup) ===================="),
    ("// 兜底：换行 → \\n", "// Dự phòng: Dòng mới -> \\n"),
]
r_rc = [
    ("// ✓ Cần trái: Kéo xuống推到底", "// ✓ Cần trái: Kéo xuống đáy"),
]

replace_in_file("board_config.h", r_board)
replace_in_file("battery.ino", r_battery)
replace_in_file("lpf.h", r_lpf)
replace_in_file("control.h", r_control_h)
replace_in_file("estimate.ino", r_estimate)
replace_in_file("util.h", r_util)
replace_in_file("imu.ino", r_imu)
replace_in_file("motors.ino", r_motors)
replace_in_file("web_rc.ino", r_webrc)
replace_in_file("rc.ino", r_rc)

print("Batch 3 done")
