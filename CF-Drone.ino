// Chương trình chính
// Code sử dụng Arduino IDE bản 2.3.x trở lên, không hỗ trợ bản cũ 1.8.19!
// Phải cài đặt core thư viện ESP32 (esp32 by Espressif Systems) và thư viện phụ thuộc (MAVLINK v.v.)
// Board mạch có thể chọn ESP32 Dev Module hoặc WeMOS D1 MINI ESP32

#include "vector.h"
#include "quaternion.h"
#include "util.h"

#define WIFI_ENABLED 1
#define WEB_RC_ENABLED 1  // Bật Web RC

float t = NAN; // current step time, s
float dt; // time delta from previous step, s
float controlRoll, controlPitch, controlYaw, controlThrottle; // pilot's inputs, range [-1, 1]
float controlMode = NAN;
Vector gyro; // gyroscope data
Vector acc; // accelerometer data, m/s/s
Vector rates; // filtered angular rates, rad/s
Quaternion attitude; // estimated attitude
bool landed; // are we landed and stationary

void setup() {
	Serial.begin(115200);
	print("Khởi tạo chương trình!\n");
	disableBrownOut();
	setupParameters();
	setupLED();
	setupMotors();
	setLED(true);
#if WIFI_ENABLED
	setupWiFi();
#endif
#if WEB_RC_ENABLED
	setupWebRC();  // Khởi tạo Web RC
#endif
	setupIMU();
	setupAltHold();
	setupRC();
	setLED(false);
	print("Khởi tạo hoàn tất!\n");
#if WEB_RC_ENABLED
	print("Địa chỉ Web RC: http://192.168.4.1:8080\n");
#endif
}

void loop() {
	readIMU();
	updateAltHold();
	step();
	readRC();
#if WEB_RC_ENABLED
	readWebRC();  // Đọc đầu vào Web RC
	processConsoleCommandQueue(); // Đưa lệnh Web vào vòng lặp chính để tránh block callback HTTP
#endif
	estimate();
	updateBatteryVoltage();
	control();
	sendMotors();
	handleInput();
#if WIFI_ENABLED
	processMavlink();
#endif
	logData();
	syncParameters();
	updateLED();
}
