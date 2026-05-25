// Điều khiển bay

#include "vector.h"
#include "quaternion.h"
#include "pid.h"
#include "lpf.h"
#include "util.h"

#if WEB_RC_ENABLED
#include "control.h"
#endif

// ============== Thông số vòng lặp Rate (vòng trong) ==============
#define PITCHRATE_P 0.05 // Tăng P để phản hồi nhanh hơn
#define PITCHRATE_I 0.2 // Giá trị I trung bình để bù sai lệch motor
#define PITCHRATE_D 0.001 // Giá trị D nhỏ để giảm dao động
#define PITCHRATE_I_LIM 0.35 // Giới hạn tích lũy I
#define ROLLRATE_P 0.06 // Sử dụng cùng thông số cho Roll và Pitch
#define ROLLRATE_I PITCHRATE_I 
#define ROLLRATE_D 0.002 
#define ROLLRATE_I_LIM PITCHRATE_I_LIM
#define YAWRATE_P 0.3 // Yaw cần giá trị P cao hơn (do quán tính nhỏ)
#define YAWRATE_I 0.01 // Giá trị I trung bình để bù trừ
#define YAWRATE_D 0.01 // Giá trị D nhỏ
#define YAWRATE_I_LIM 0.3
// ============== Thông số vòng lặp Angle (vòng ngoài) ==============
#define ROLL_P 7 // Giá trị P cao để phản hồi nhanh
#define ROLL_I 0 // Vòng lặp Angle thường không cần giá trị I
#define ROLL_D 0 // Vòng lặp Angle thường không cần giá trị D
#define PITCH_P ROLL_P // Tương tự cho Roll và Pitch
#define PITCH_I ROLL_I
#define PITCH_D ROLL_D
#define YAW_P 3 // Phản hồi Yaw hơi chậm hơn
// ============== Giá trị giới hạn ==============
#define PITCHRATE_MAX radians(360) // Giới hạn tốc độ quay tối đa (1000°/s)
#define ROLLRATE_MAX radians(360)
#define YAWRATE_MAX radians(300) // Tốc độ quay Yaw chậm hơn một chút
#define TILT_MAX radians(30) // Góc nghiêng tối đa 30°
#define ALTHOLD_HOVER_THRUST 0.5f   // Lực đẩy cơ bản khi lơ lửng ở chế độ Giữ độ cao (sẽ thay thế khi có cảm biến áp suất)
#define ARM_THROTTLE_LIMIT   0.05f  // Giới hạn ga khi mở khóa (sau khi chuẩn hóa 0~1), 5%, vượt quá sẽ cấm mở khóa
#define RATES_D_LPF_ALPHA 0.2 // cutoff frequency ~ 40 Hz

float motThrMin = 0.10f;  // Giới hạn lực đẩy tối thiểu (ngõ ra khi cần ga ở mức thấp nhất), có thể chỉnh qua thông số MOT_THR_MIN
float motThrMax = 1.0f;   // Giới hạn lực đẩy tối đa (ngõ ra khi cần ga ở mức cao nhất), có thể chỉnh qua thông số MOT_THR_MAX

const int RAW = 0, ACRO = 1, STAB = 2, ALTHOLD = 3, AUTO = 4; // flight modes
int mode = STAB;
bool armed = false;
int flightModes[] = {STAB, STAB, STAB}; // Chế độ bay tương ứng với 3 mức công tắc trên RC, có thể cấu hình qua thông số CTL_FLT_MODE_0/1/2

#if WEB_RC_ENABLED
extern uint16_t webRCButtons;
#endif

PID rollRatePID(ROLLRATE_P, ROLLRATE_I, ROLLRATE_D, ROLLRATE_I_LIM, RATES_D_LPF_ALPHA);
PID pitchRatePID(PITCHRATE_P, PITCHRATE_I, PITCHRATE_D, PITCHRATE_I_LIM, RATES_D_LPF_ALPHA);
PID yawRatePID(YAWRATE_P, YAWRATE_I, YAWRATE_D);
PID rollPID(ROLL_P, ROLL_I, ROLL_D);
PID pitchPID(PITCH_P, PITCH_I, PITCH_D);
PID yawPID(YAW_P, 0, 0);
Vector maxRate(ROLLRATE_MAX, PITCHRATE_MAX, YAWRATE_MAX);
float tiltMax = TILT_MAX;

Quaternion attitudeTarget;
Vector ratesTarget;
Vector ratesExtra; // feedforward rates
Vector torqueTarget;
float thrustTarget;

extern const int MOTOR_REAR_LEFT, MOTOR_REAR_RIGHT, MOTOR_FRONT_RIGHT, MOTOR_FRONT_LEFT;
extern float motors[4];
extern float controlRoll, controlPitch, controlThrottle, controlYaw, controlMode;
extern float batteryVoltage;  // battery.ino

void control() {
	interpretControls();
#if WEB_RC_ENABLED
	interpretWebRC();
#endif
	failsafe();
	controlAttitude();
	controlRates();
	controlTorque();
}

void interpretControls() {
	if (controlMode < 0.25) mode = flightModes[0];
	else if (controlMode <= 0.75) mode = flightModes[1];
	else if (controlMode > 0.75) mode = flightModes[2];

	if (mode == AUTO) return; // pilot is not effective in AUTO mode

#if WEB_RC_ENABLED
	if (!isUsingWebRC()) { // Cử chỉ mở khóa SBUS chỉ có tác dụng khi WebRC không hoạt động
#endif
	static bool armWarnNotified = false;  // Chống spam log: Thông báo cấm mở khóa khi pin yếu
	if (controlThrottle < 0.05 && controlYaw > 0.95) { // arm gesture
		if (batteryVoltage > VBAT_ABSENT_THRESHOLD && batteryVoltage < VBAT_WARN_THRESHOLD) {
			// Mức L1 và thấp hơn: Cấm mở khóa, thông báo khi trạng thái thay đổi
			if (!armWarnNotified) {
				print("Pin yếu(%.2fV), cấm mở khóa\n", batteryVoltage);
#if WEB_RC_ENABLED
				char warnBuf[64];
				snprintf(warnBuf, sizeof(warnBuf), "Pin yếu(%.2fV) Cấm mở khóa", batteryVoltage);
				setWebRCWarn(warnBuf);
#endif
				armWarnNotified = true;
			}
		} else {
			armed = true;
			armWarnNotified = false; // Reset sau khi thay pin mới
		}
	}
	if (controlThrottle < 0.05 && controlYaw < -0.95) armed = false; // disarm gesture
#if WEB_RC_ENABLED
	}
#endif

	if (abs(controlYaw) < 0.1) controlYaw = 0; // yaw dead zone

	if (controlThrottle < 0.05f) {
		thrustTarget = 0.0f;   // Vùng chết đáy -> Chạy không tải (idle), PID không hoạt động
	} else {
		thrustTarget = mapf(controlThrottle, 0.05f, 1.0f, motThrMin, motThrMax);
	}

	if (mode == STAB) {
		float yawTarget = attitudeTarget.getYaw();
		if (!armed || invalid(yawTarget) || controlYaw != 0) yawTarget = attitude.getYaw(); // reset yaw target
		attitudeTarget = Quaternion::fromEuler(Vector(controlRoll * tiltMax, controlPitch * tiltMax, yawTarget));
		ratesExtra = Vector(0, 0, -controlYaw * maxRate.z); // positive yaw stick means clockwise rotation in FLU
	}

	if (mode == ACRO) {
		attitudeTarget.invalidate(); // skip attitude control
		ratesTarget.x = controlRoll * maxRate.x;
		ratesTarget.y = controlPitch * maxRate.y;
		ratesTarget.z = -controlYaw * maxRate.z; // positive yaw stick means clockwise rotation in FLU
	}

	if (mode == RAW) { // direct torque control
		attitudeTarget.invalidate(); // skip attitude control
		ratesTarget.invalidate(); // skip rate control
		torqueTarget = Vector(controlRoll, controlPitch, -controlYaw) * 0.1;
	}
}

void controlAttitude() {
	if (!armed || attitudeTarget.invalid() || thrustTarget < motThrMin) return; // skip attitude control

	const Vector up(0, 0, 1);
	Vector upActual = Quaternion::rotateVector(up, attitude);
	Vector upTarget = Quaternion::rotateVector(up, attitudeTarget);

	Vector error = Vector::rotationVectorBetween(upTarget, upActual);

	ratesTarget.x = rollPID.update(error.x) + ratesExtra.x;
	ratesTarget.y = pitchPID.update(error.y) + ratesExtra.y;

	float yawError = wrapAngle(attitudeTarget.getYaw() - attitude.getYaw());
	ratesTarget.z = yawPID.update(yawError) + ratesExtra.z;
}


void controlRates() {
	if (!armed || ratesTarget.invalid() || thrustTarget < motThrMin) return; // skip rates control

	Vector error = ratesTarget - rates;

	// Calculate desired torque, where 0 - no torque, 1 - maximum possible torque
	torqueTarget.x = rollRatePID.update(error.x);
	torqueTarget.y = pitchRatePID.update(error.y);
	torqueTarget.z = yawRatePID.update(error.z);
}

void controlTorque() {
	if (!torqueTarget.valid()) return; // skip torque control

	if (!armed) {
		memset(motors, 0, sizeof(motors)); // stop motors if disarmed
		return;
	}

	if (thrustTarget < motThrMin) {
		for (int i = 0; i < 4; i++) motors[i] = motThrMin; // idle thrust
		return;
	}

	motors[MOTOR_FRONT_LEFT] = thrustTarget + torqueTarget.x - torqueTarget.y + torqueTarget.z;
	motors[MOTOR_FRONT_RIGHT] = thrustTarget - torqueTarget.x - torqueTarget.y - torqueTarget.z;
	motors[MOTOR_REAR_LEFT] = thrustTarget + torqueTarget.x + torqueTarget.y - torqueTarget.z;
	motors[MOTOR_REAR_RIGHT] = thrustTarget - torqueTarget.x + torqueTarget.y + torqueTarget.z;

	desaturate(motors[MOTOR_FRONT_LEFT], motors[MOTOR_FRONT_RIGHT], motors[MOTOR_REAR_LEFT], motors[MOTOR_REAR_RIGHT]);

	motors[0] = constrain(motors[0], 0, 1);
	motors[1] = constrain(motors[1], 0, 1);
	motors[2] = constrain(motors[2], 0, 1);
	motors[3] = constrain(motors[3], 0, 1);
}

void desaturate(float& a, float& b, float& c, float& d) {
	float maxThrust = max(max(a, b), max(c, d));
	if (maxThrust > 1) {
		float diff = maxThrust - 1;
		a -= diff;
		b -= diff;
		c -= diff;
		d -= diff;
	}
}

const char* getModeName() {
	switch (mode) {
		case RAW:     return "RAW";
		case ACRO:    return "ACRO";
		case STAB:    return "STAB";
		case ALTHOLD: return "ALTHOLD";
		case AUTO:    return "AUTO";
		default:      return "UNKNOWN";
	}
}

#if WEB_RC_ENABLED
// Xử lý nút điều khiển trên WebRC (Chỉ phụ trách ARM/DISARM và chuyển chế độ)
void interpretWebRC() {
	if (!isUsingWebRC()) return;

	// Xử lý nút mở khóa/khóa (Phát hiện sườn lên để tránh kích hoạt lặp lại mỗi chu kỳ)
	static uint16_t lastWebRCButtons = 0;
	uint16_t risingEdge = webRCButtons & ~lastWebRCButtons;
	lastWebRCButtons = webRCButtons;

	// Log trạng thái thay đổi mở khóa/khóa
	static bool lastArmedState = false;
	if (armed != lastArmedState) {
		print(armed ? "Web RC: Đã mở khóa\n" : "Web RC: Đã khóa\n");
		lastArmedState = armed;
	}

	// Nút 0: Mở khóa (Sườn lên)
	if (risingEdge & 0x0001) {
		if (controlThrottle < ARM_THROTTLE_LIMIT) {
			armed = true;
		} else {
			setWebRCWarn("Ga quá cao, không thể mở khóa");
		}
	}

	// Nút 1: Khóa (Sườn lên)
	if (risingEdge & 0x0002) {
		armed = false;
	}

	// Nút 2: Dừng khẩn cấp (Sườn lên)
	if (risingEdge & 0x0004) {
		armed = false;
		thrustTarget = 0.0f;
	}

	// Nút 6: Chế độ Cân bằng (STAB) (Sườn lên)
	if (risingEdge & 0x0040) {
		mode = STAB;
	}

	// Nút 7: Chế độ Nhào lộn (ACRO) (Sườn lên)
	if (risingEdge & 0x0080) {
		mode = ACRO;
	}

	// Nút 8: Chế độ Giữ độ cao (ALTHOLD) (Sườn lên) - Chưa hỗ trợ, chuyển về STAB để tránh lỗi tuần hoàn
	if (risingEdge & 0x0100) {
		mode = STAB;
		setWebRCWarn("Chế độ Giữ độ cao chưa hỗ trợ, đã chuyển sang Cân bằng");
	}

	// Log thay đổi chế độ bay
	static int lastMode = STAB;
	if (mode != lastMode) {
		print("Web RC: Chế độ chuyển sang %s\n", getModeName());
		lastMode = mode;
	}
}
#endif
