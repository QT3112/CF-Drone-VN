// Phát hiện điện áp pin
// Battery voltage monitoring

#include "board_config.h"

#define VBAT_ADC_PIN       BOARD_VBAT_ADC_PIN  // Chân lấy mẫu điện áp pin
#define VBAT_ADC_SAMPLES   16               // Lấy trung bình nhiều mẫu ADC, giá trị càng lớn thì nhiễu càng thấp, phản hồi càng chậm
#define VBAT_DIVIDER       (43.0f / 33.0f)  // Tỷ lệ chia áp: Điện trở trên 10kΩ + dưới 33kΩ, công thức Vbat = Vadc × 43/33
#define VBAT_WARN_THRESHOLD     3.5f  // L1: Ngưỡng cảnh báo không tải/idle (V)
#define VBAT_LOW_THRESHOLD      3.3f  // L2: Ngưỡng cảnh báo pin yếu khi đang bay (V)
#define VBAT_CRITICAL_THRESHOLD 3.0f  // L3: Ngưỡng nguy hiểm khi bay, tự động hạ cánh (V)
#define VBAT_ABSENT_THRESHOLD   0.5f  // Thấp hơn giá trị này được xem là không lắp pin, bỏ qua mọi cảnh báo

#define BATTERY_FLYING_THRUST_MIN    0.15f  // Lực đẩy >= giá trị này xem như đang bay, L1 không áp dụng
#define BATTERY_ACTION_DEBOUNCE_TIME  0.9f  // Thời gian debounce để xác nhận pin yếu liên tục (giây)

extern float t;
float batteryVoltage = 0.0f;  // Điện áp mới nhất toàn cầu, được cập nhật định kỳ bởi updateBatteryVoltage()

void updateBatteryVoltage() {
	static float lastCheck = 0;
	if (t - lastCheck < 0.5f) return;  // Lấy mẫu mỗi 0.5 giây
	lastCheck = t;
	batteryVoltage = readBatteryVoltage();
}

float readBatteryVoltage() {
	long sum = 0;
	for (int i = 0; i < VBAT_ADC_SAMPLES; i++) sum += analogReadMilliVolts(VBAT_ADC_PIN);
	return (sum / (float)VBAT_ADC_SAMPLES / 1000.0f) * VBAT_DIVIDER;
}
