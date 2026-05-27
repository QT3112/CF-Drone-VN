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
#define PITCHRATE_MAX radians(120) // Tốc độ quay tối đa (an toàn cho người mới)
#define ROLLRATE_MAX  radians(120)
#define YAWRATE_MAX   radians(90)  // Yaw chậm hơn cho ổn định
#define TILT_MAX      radians(15)  // Giới hạn góc nghiêng 15° (an toàn)
#define ARM_THROTTLE_LIMIT 0.05f   // Giới hạn ga khi mở khóa
#define RATES_D_LPF_ALPHA  0.2     // cutoff frequency ~ 40 Hz

float motThrMin = 0.10f;  // Giới hạn lực đẩy tối thiểu (ngõ ra khi cần ga ở mức thấp nhất), có thể chỉnh qua thông số MOT_THR_MIN
float motThrMax = 1.0f;   // Giới hạn lực đẩy tối đa (ngõ ra khi cần ga ở mức cao nhất), có thể chỉnh qua thông số MOT_THR_MAX

const int ALTHOLD = 3, AUTO = 4; // Chế độ bay (ALTHOLD là chế độ duy nhất)
int mode   = ALTHOLD;              // Luôn ở ALTHOLD
bool armed = false;

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
extern float altholdThrustTarget; // althold.ino

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
	// Chế độ luôn là ALTHOLD, không cần chọn từ công tắc RC
	if (mode != AUTO) mode = ALTHOLD;
	if (mode == AUTO) return; // AUTO do failsafe kích hoạt, phi công không can thiệp

	// Vùng chết Yaw
	if (abs(controlYaw) < 0.1) controlYaw = 0;

	// Thrust từ ALTHOLD PID
	if (altholdThrustTarget < 0.05f) {
		thrustTarget = 0.0f;
	} else {
		thrustTarget = mapf(altholdThrustTarget, 0.05f, 1.0f, motThrMin, motThrMax);
	}

	// Đặt hướng drone (Attitude target) dựa trên joystick phải + Yaw
	{
		float yawTarget = attitudeTarget.getYaw();
		if (!armed || invalid(yawTarget) || controlYaw != 0)
			yawTarget = attitude.getYaw(); // Reset yaw target nếu mới arm hoặc đang quay
		attitudeTarget = Quaternion::fromEuler(
			Vector(controlRoll * tiltMax, controlPitch * tiltMax, yawTarget));
		ratesExtra = Vector(0, 0, -controlYaw * maxRate.z);
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

	if (thrustTarget <= 0.0f) {
		memset(motors, 0, sizeof(motors)); // Tắt hoàn toàn (AH_IDLE hoặc cú hạ cánh)
		return;
	}

	if (thrustTarget < motThrMin) {
		for (int i = 0; i < 4; i++) motors[i] = motThrMin; // idle spin (không thể xảy ra với althold mới)
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
		case ALTHOLD: return "ALTHOLD";
		case AUTO:    return "AUTO";
		default:      return "UNKNOWN";
	}
}

#if WEB_RC_ENABLED
// Xử lý nút bấm Web RC (chỉ ARM / DISARM / DừNG KHẨN CẤP)
void interpretWebRC() {
	if (!isUsingWebRC()) return;

	// Phát hiện sườn lên của nút bấm
	static uint16_t lastWebRCButtons = 0;
	uint16_t risingEdge  = webRCButtons & ~lastWebRCButtons;
	lastWebRCButtons     = webRCButtons;

	// Log khi trạng thái arm thay đổi
	static bool lastArmedState = false;
	if (armed != lastArmedState) {
		print(armed ? "Web RC: Da mo khoa\n" : "Web RC: Da khoa\n");
		lastArmedState = armed;
	}

	// Nút 0: Mở khóa
	if (risingEdge & 0x0001) {
		if (batteryVoltage > VBAT_WARN_THRESHOLD || batteryVoltage < VBAT_ABSENT_THRESHOLD) {
			armed = true;
			print("Web RC: Mo khoa\n");
		} else {
			setWebRCWarn("Pin yeu, khong the mo khoa");
		}
	}

	// Nút 1: Khóa
	if (risingEdge & 0x0002) {
		armed = false;
	}

	// Nút 2: Dừng khẩn cấp
	if (risingEdge & 0x0004) {
		armed        = false;
		thrustTarget = 0.0f;
		print("Web RC: DUNG KHAN CAP!\n");
	}
}
#endif
