// Điều khiển đèn LED trên mạch
// Board's LED control

#include "board_config.h"

#if BOARD_LED_ENABLED

#define BLINK_PERIOD      500000  // Nháy chậm: Chu kỳ 500ms -> 1 Hz
#define BLINK_FAST_PERIOD  62500  // Nháy nhanh: Chu kỳ 62.5ms -> 8 Hz

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // for ESP32 Dev Module
#endif

// ---- Khai báo phụ thuộc bên ngoài ----
extern bool armed;
extern float t;
extern float controlTime;
extern float rcLossTimeout;
extern float thrustTarget;    // control.ino
extern float batteryVoltage;  // battery.ino
extern bool isInverted; // safety.ino
#if WEB_RC_ENABLED
extern bool webRCEnabled;
extern bool useWebRC;
bool isUsingWebRC();
#endif

void setupLED() {
	pinMode(LED_BUILTIN, OUTPUT);
}

void setLED(bool on) {
	static bool state = false;
	if (on == state) {
		return; // don't call digitalWrite if the state is the same
	}
	digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
	state = on;
}

void blinkLED() {
	setLED(micros() / BLINK_PERIOD % 2);
}

// Cảnh báo pin: Chọn ngưỡng theo trạng thái bay
// Đang bay (thrustTarget >= 0.15) -> L2 (2.8V), L1 không áp dụng khi bay
// Chưa mở khóa / Đang idle -> L1 (3.4V)
bool batteryAlertActive() {
	if (batteryVoltage <= VBAT_ABSENT_THRESHOLD) return false;
	bool flying = armed && thrustTarget >= 0.15f;
	if (flying) return batteryVoltage < VBAT_LOW_THRESHOLD;   // L2: Đang bay
	else        return batteryVoltage < VBAT_WARN_THRESHOLD;   // L1: Chưa mở khóa / Đang idle
}

// Kiểm tra xem có cảnh báo nào không (Pin yếu / Mất RC / Lật ngược)
bool ledAlertActive() {
	// Phát hiện lật ngược
	if (isInverted) return true;

	// Phát hiện mất kết nối RC (SBUS RC, chỉ sau khi mở khóa)
	if (controlTime != 0 && armed && (t - controlTime > rcLossTimeout)) return true;

#if WEB_RC_ENABLED
	// Phát hiện mất kết nối Web RC: Đã kích hoạt nhưng timeout
	if (webRCEnabled && useWebRC && !isUsingWebRC()) return true;
#endif

	if (batteryAlertActive()) return true;

	return false;
}

// Gọi trong vòng lặp chính: Điều khiển LED dựa trên trạng thái bay
void updateLED() {
	if (!armed) {
		if (batteryAlertActive()) {
			setLED(micros() / BLINK_FAST_PERIOD % 2); // Pin yếu trước khi mở khóa: Nháy nhanh
		} else {
			setLED(false); // Chờ bình thường: Tắt
		}
		return;
	}
	if (ledAlertActive()) {
		setLED(micros() / BLINK_FAST_PERIOD % 2); // Bất kỳ cảnh báo nào: Nháy nhanh 8Hz
	} else {
		setLED(micros() / BLINK_PERIOD % 2); // Bay bình thường: Nháy chậm 1Hz
	}
}

#else // BOARD_LED_ENABLED == 0 (ESP32-C3 không có LED trên mạch)

void setupLED() {}
void setLED(bool on) {}
void updateLED() {}

#endif // BOARD_LED_ENABLED
