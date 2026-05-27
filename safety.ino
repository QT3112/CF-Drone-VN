// Bảo vệ an toàn lỗi (Failsafe)
// Fail-safe functions

bool isInverted = false;  // Thân máy bay đang bị lật ngược? (cos trục Z < INVERTED_COS_THRESHOLD)

float rcLossTimeout = 1;        // Thời gian timeout mất kết nối RC (giây), cấu hình qua SF_RC_LOSS_TIME
float descendTime = 10;         // Thời gian hạ cánh đến khi tắt máy (giây), cấu hình qua SF_DESCEND_TIME
#define WEB_RC_LOSS_TIMEOUT_MS 8000UL  // Ngưỡng mất kết nối WebRC (ms), phải lớn hơn khoảng thời gian heartbeat 2000ms

// Thông số bảo vệ lật ngược
#define INVERTED_COS_THRESHOLD -0.7f   // cos(134°), góc nghiêng > 134° xem như lật ngược (đã bù trừ sai số gyro)
#define INVERTED_TIMEOUT        1.5f   // Giữ trạng thái lật ngược > 1.5s sẽ tắt máy

extern float controlTime;
extern float controlRoll, controlPitch, controlThrottle, controlYaw;

#if WEB_RC_ENABLED
// Khai báo biến Web RC
extern bool webRCEnabled;
extern bool useWebRC;
extern unsigned long webRCLastUpdate;
bool isUsingWebRC();
#endif

extern bool armed;
extern int mode;
extern float dt;
extern float thrustTarget;
extern float t;
extern float batteryVoltage;  // battery.ino
extern Quaternion attitudeTarget;
extern Quaternion attitude;

void failsafe() {
	rcLossFailsafe();
#if WEB_RC_ENABLED
	webRCLossFailsafe();
#endif
	autoFailsafe();
	invertedFailsafe();
	batteryFailsafe();
}

// RC loss failsafe
void rcLossFailsafe() {
	if (controlTime == 0) return; // no RC at all
	if (!armed) return;
#if WEB_RC_ENABLED
	if (isUsingWebRC()) return; // WebRC tự xử lý timeout độc lập (webRCLossFailsafe)
#endif
	if (t - controlTime > rcLossTimeout) {
		descend();
	}
}

// Smooth descend on RC lost
void descend() {
	mode = AUTO;
	attitudeTarget = Quaternion();
	thrustTarget -= dt / descendTime;
	if (thrustTarget < 0) {
		thrustTarget = 0;
		armed = false;
	}
}

// Allow pilot to interrupt automatic flight
void autoFailsafe() {
	static float roll, pitch, yaw, throttle;
	
	// control* đã bao gồm đầu vào SBUS/MAVLink/WebRC, có thể kiểm tra trực tiếp
	if (roll != controlRoll || pitch != controlPitch || yaw != controlYaw || abs(throttle - controlThrottle) > 0.05) {
		if (mode == AUTO) mode = ALTHOLD; // Trở về ALTHOLD khi phi công can thiệp
	}
	
	roll = controlRoll;
	pitch = controlPitch;
	yaw = controlYaw;
	throttle = controlThrottle;
}

#if WEB_RC_ENABLED
// Bảo vệ mất kết nối Web RC
void webRCLossFailsafe() {
	if (!webRCEnabled || !useWebRC) return;
	if (!armed) return;

	// So sánh trực tiếp bằng ms, tránh sai số 1s khi chia số nguyên
	if (millis() - webRCLastUpdate > WEB_RC_LOSS_TIMEOUT_MS) {
		print("Mất kết nối Web RC, bắt đầu hạ cánh\n");
		descend();
		webRCEnabled = false;
		useWebRC = false;
	}
}
#endif

// Bảo vệ lật ngược: Góc giữa trục Z máy bay và trục Z thế giới > 120° trong 1.5s -> tắt máy
void invertedFailsafe() {
	if (!armed) {
		isInverted = false;
		return;
	}

	// Lấy trục Z của máy bay trên hệ tọa độ thế giới: Đứng thẳng ≈ +1, lật ngược ≈ -1
	Vector worldUp = Quaternion::rotateVector(Vector(0, 0, 1), attitude);

	static float invertedStartTime = 0;
	if (worldUp.z < INVERTED_COS_THRESHOLD) {
		isInverted = true;
		if (invertedStartTime == 0) invertedStartTime = t;
		if (t - invertedStartTime > INVERTED_TIMEOUT) {
			armed = false;
			thrustTarget = 0.0f;
			invertedStartTime = 0;
			print("Bảo vệ lật ngược: Đã dừng\n");
		}
	} else {
		isInverted = false;
		invertedStartTime = 0;
	}
}

// Bảo vệ điện áp pin
// L1 (3.4V): Chưa mở khóa thì cấm mở khóa (xử lý ở control.ino), đang idle tự động khóa
// L2 (2.8V): Đang bay thì chỉ nhấp nháy LED cảnh báo nhanh
// L3 (2.6V): Đang bay thì tự động hạ cánh (dùng descend(), tốc độ do SF_DESCEND_TIME quyết định)
void batteryFailsafe() {
	static bool l3Latched = false;
	static float lowSince = 0.0f;
	static float l3LastNotify = 0.0f;

	if (batteryVoltage < VBAT_ABSENT_THRESHOLD) return; // Không kết nối pin, bỏ qua
	if (!armed) {
		l3Latched = false;
		lowSince = 0.0f;
		l3LastNotify = 0.0f;
		return;
	}

	// L3 đã kích hoạt thì tiếp tục hạ cánh cho đến khi khóa, tránh lặp đi lặp lại ở ngưỡng cắt
	if (l3Latched) {
		if (t - l3LastNotify >= 2.0f) {
			print("Pin cực yếu(%.2fV), đang hạ cánh, lực đẩy %.0f%%\n",
			      batteryVoltage, thrustTarget * 100.0f);
#if WEB_RC_ENABLED
			char warnBuf[64];
			snprintf(warnBuf, sizeof(warnBuf),
			         "Pin cực yếu(%.2fV) Đang hạ cánh, Lực đẩy %.0f%%",
			         batteryVoltage, thrustTarget * 100.0f);
			setWebRCWarn(warnBuf);
#endif
			l3LastNotify = t;
		}
		descend();
		return;
	}

	bool flying = thrustTarget >= BATTERY_FLYING_THRUST_MIN;

	bool actionCondition = false;
	bool criticalAction = false;

	// Đang bay: Chỉ L3 kích hoạt hành động mạnh; L2 tiếp tục cảnh báo bằng LED
	if (flying) {
		if (batteryVoltage < VBAT_CRITICAL_THRESHOLD) {
			actionCondition = true;
			criticalAction = true;
		}
	} else {
		// Mở khóa nhưng đang idle: L1 kích hoạt tự động khóa
		if (batteryVoltage < VBAT_WARN_THRESHOLD) {
			actionCondition = true;
		}
	}

	if (!actionCondition) {
		lowSince = 0.0f;
		return;
	}

	if (lowSince == 0.0f) lowSince = t;
	if (t - lowSince < BATTERY_ACTION_DEBOUNCE_TIME) return;

	lowSince = 0.0f;
	if (criticalAction) {
		l3Latched = true;
		l3LastNotify = 0.0f;
		print("Pin cực yếu(%.2fV), tự động hạ cánh\n", batteryVoltage);
#if WEB_RC_ENABLED
		char warnBuf[64];
		snprintf(warnBuf, sizeof(warnBuf),
		         "Pin cực yếu(%.2fV) Tự động hạ cánh",
		         batteryVoltage);
		setWebRCWarn(warnBuf);
#endif
		descend();
		return;
	}

	armed = false;
	thrustTarget = 0.0f;
	print("Pin yếu(%.2fV), tự động khóa\n", batteryVoltage);
#if WEB_RC_ENABLED
	char warnBuf[64];
	snprintf(warnBuf, sizeof(warnBuf), "Pin yếu(%.2fV) Đã tự động khóa", batteryVoltage);
	setWebRCWarn(warnBuf);
#endif
}
