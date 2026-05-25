// Điều khiển ngõ ra motor dùng MOSFET
// Nếu dùng ESC, hãy đổi pwmStop, pwmMin và pwmMax sang giá trị us phù hợp, giảm pwmFrequency xuống 400
// Motors output control using MOSFETs
// In case of using ESCs, change pwmStop, pwmMin and pwmMax to appropriate values in μs, decrease pwmFrequency (to 400)

#include "util.h"
#include "board_config.h"

float motors[4]; // normalized motor thrusts in range [0..1]

// Chân motor (Tương ứng MOTOR_REAR_LEFT=0, MOTOR_REAR_RIGHT=1, MOTOR_FRONT_RIGHT=2, MOTOR_FRONT_LEFT=3)
int motorPins[4] = BOARD_MOTOR_PINS; // RL, RR, FR, FL

int pwmFrequency = 25000; // Clock XTAL mặc định của ESP32-C3 LEDC là 40MHz, giới hạn 10-bit là 39062Hz;
                          // ESP32 APB 80MHz, giới hạn 10-bit là 78125Hz;
                          // Ở tần số 25kHz, prescaler dưới cả XTAL/APB đều có thể biểu diễn chính xác, dư dả an toàn
int pwmResolution = 10;
int pwmStop = 0;
int pwmMin = 0;
int pwmMax = -1; // -1 là chế độ Duty Cycle thuần túy (Nối MOSFET trực tiếp); Khi nối ESC thì đặt là giá trị PWM tối đa thực tế (us)

// Motors array indexes:
const int MOTOR_REAR_LEFT = 0;
const int MOTOR_REAR_RIGHT = 1;
const int MOTOR_FRONT_RIGHT = 2;
const int MOTOR_FRONT_LEFT = 3;

void setupMotors() {
	print("Setup Motors\n");

	// Giải phóng tất cả các chân trước (xóa kênh LEDC cũ khi gọi lặp lại), sau đó kéo xuống mức thấp để chống quay nhầm
	for (int i = 0; i < 4; i++) {
		ledcDetach(motorPins[i]);         // Lần đầu gọi nếu không có gán chân sẽ tự động trả về false, không có tác dụng phụ
		pinMode(motorPins[i], OUTPUT);
		digitalWrite(motorPins[i], LOW);
	}

	// configure pins
	for (int i = 0; i < 4; i++) {
		bool ok = ledcAttach(motorPins[i], pwmFrequency, pwmResolution);
		if (ok) {
			double actual = ledcChangeFrequency(motorPins[i], pwmFrequency, pwmResolution);
			if (actual > 0) pwmFrequency = (int)round(actual); // Dùng double để nhận giá trị trả về, tránh mất độ chính xác
		}
		print("  motor%d pin=%d ledcAttach=%s\n", i, motorPins[i], ok ? "OK" : "FAIL");
	}

	sendMotors();
	print("Motors initialized\n");
}

int getDutyCycle(float value) {
	value = constrain(value, 0, 1);
	if (pwmMax >= 0) { // Chế độ thời gian PWM (Dùng cho nối ESC)
		float pwm = mapf(value, 0, 1, pwmMin, pwmMax);
		if (value == 0) pwm = pwmStop;
		float duty = mapf(pwm, 0, 1000000 / pwmFrequency, 0, (1 << pwmResolution) - 1);
		return round(duty);
	} else { // Chế độ Duty Cycle thuần túy (Dùng cho nối MOSFET trực tiếp)
		return round(value * ((1 << pwmResolution) - 1));
	}
}

void sendMotors() {
	for (int i = 0; i < 4; i++) {
		ledcWrite(motorPins[i], getDutyCycle(motors[i]));
	}
}

bool motorsActive() {
	return motors[0] != 0 || motors[1] != 0 || motors[2] != 0 || motors[3] != 0;
}

void testMotor(int n) {
	print("Testing motor %d\n", n);
	motors[n] = 0.3;
	delay(50); // ESP32 may need to wait until the end of the current cycle to change duty https://github.com/espressif/arduino-esp32/issues/5306
	sendMotors();
	pause(3);
	motors[n] = 0;
	sendMotors();
	print("Done\n");
}
