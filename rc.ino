// Sử dụng bộ thu RC (Receiver)
// Work with the RC receiver

#include <SBUS.h>
#include "util.h"
#include "board_config.h"

SBUS rc(BOARD_RC_SERIAL); // Serial2 on ESP32, Serial1 on ESP32-C3
int rcRxPin = BOARD_RC_RX_PIN; // Chân SBUS RX, -1 là vô hiệu hóa RC, có thể cấu hình qua tham số RC_RX_PIN
uint16_t channels[16]; // raw rc channels
float controlTime; // time of the last controls update
float channelZero[16]; // calibration zero values Giá trị "điểm 0" của mỗi kênh (giá trị nhỏ nhất/trung tâm)
float channelMax[16]; // calibration max values Giá trị "tối đa" của mỗi kênh

// Channels mapping (using float to store in parameters):
float rollChannel = NAN, pitchChannel = NAN, throttleChannel = NAN, yawChannel = NAN, modeChannel = NAN;

void setupRC() {
	if (rcRxPin < 0) return; // Bỏ qua khởi tạo RC nếu chân (Pin) chưa được cấu hình
	print("Setup RC\n");
	rc.begin(rcRxPin);
}

bool readRC() {
	if (rcRxPin < 0) return false; // RC chưa được bật
	if (rc.read()) {
		SBUSData data = rc.data();
		for (int i = 0; i < 16; i++) channels[i] = data.ch[i]; // copy channels data
		normalizeRC();
		controlTime = t;
		return true;
	}
	return false;
}

void normalizeRC() {
	float controls[16];
	for (int i = 0; i < 16; i++) {
		controls[i] = mapf(channels[i], channelZero[i], channelMax[i], 0, 1);
	}
	// Update control values
	controlRoll = rollChannel >= 0 ? controls[(int)rollChannel] : NAN;
	controlPitch = pitchChannel >= 0 ? controls[(int)pitchChannel] : NAN;
	controlYaw = yawChannel >= 0 ? controls[(int)yawChannel] : NAN;
	controlThrottle = throttleChannel >= 0 ? controls[(int)throttleChannel] : NAN;
	controlMode = modeChannel >= 0 ? controls[(int)modeChannel] : NAN;
}

void calibrateRC() {
	uint16_t zero[16];
	uint16_t center[16];
	uint16_t max[16];
	print("1/8 Cân chỉnh Remote, đưa tất cả cần về giữa [3 giây]\n");
	pause(8);
	calibrateRCChannel(NULL, zero, zero, "2/8 Cần trái: Kéo xuống đáy, Cần phải: Giữa [3 giây]\n...     ...\n...     .o.\n.o.     ...\n");
	calibrateRCChannel(NULL, center, center, "3/8 Cần trái: Giữa, Cần phải: Giữa [3 giây]\n...     ...\n.o.     .o.\n...     ...\n");
	calibrateRCChannel(&throttleChannel, zero, max, "4/8 Xác định kênh Ga, Cần trái: Đẩy lên đỉnh (Ga tối đa), Cần phải: Giữa [3 giây]\n.o.     ...\n...     .o.\n...     ...\n");
	calibrateRCChannel(&yawChannel, center, max, "5/8 Xác định kênh Xoay, Cần trái: Đẩy hết sang phải (Xoay phải), Cần phải: Giữa [3 giây]\n...     ...\n..o     .o.\n...     ...\n");
	calibrateRCChannel(&pitchChannel, zero, max, "6/8 Xác định kênh Tiến/Lùi, Cần trái: Kéo xuống đáy, Cần phải: Đẩy lên đỉnh (Tiến tới) [3 giây]\n...     .o.\n...     ...\n.o.     ...\n");
	calibrateRCChannel(&rollChannel, zero, max, "7/8 Xác định kênh Nghiêng, Cần trái: Kéo xuống đáy, Cần phải: Đẩy hết sang phải (Nghiêng phải) [3 giây]\n...     ...\n...     ..o\n.o.     ...\n");
	calibrateRCChannel(&modeChannel, zero, max, "8/8 Xác định kênh Chế độ bay, gạt công tắc mở khóa về vị trí khóa, sau đó gạt công tắc chế độ bay lên mức cao nhất [3 giây]\n");
	printRCCalibration();
}

// Bước 1: Vị trí khởi tạo, Thao tác:
// ✓ Tất cả cần đưa về giữa
// ✓ Tất cả công tắc gạt về vị trí mặc định (thường là khóa/giữa)
// ✓ Giữ yên trong 3 giây
// Bước 2: Di chuyển cần cơ bản, Thao tác:
// ✓ Cần trái: Kéo xuống
// ✓ Cần phải: Giữa
// ✓ Giữ trong 3 giây
// Bước 3: Đưa cần về giữa, Thao tác:
// ✓ Cần trái: Giữa
// ✓ Cần phải: Giữa  
// ✓ Giữ trong 3 giây
// Bước 4: Nhận diện kênh Ga, Thao tác:
// ✓ Cần trái: Đẩy lên cao nhất (Ga lớn nhất)
// ✓ Cần phải: Giữa
// ✓ Giữ trong 3 giây
// -> Hệ thống tự động nhận diện kênh Ga
// Bước 5: Nhận diện kênh Xoay, Thao tác:
// ✓ Cần trái: Đẩy hết sang phải (Xoay phải)
// ✓ Cần phải: Giữa
// ✓ Giữ trong 3 giây
// -> Hệ thống tự động nhận diện kênh Xoay
// Bước 6: Nhận diện kênh Tiến/Lùi, Thao tác:
// ✓ Cần trái: Kéo xuống đáy
// ✓ Cần phải: Đẩy lên cao nhất (Tiến tới)
// ✓ Giữ trong 3 giây
// -> Hệ thống tự động nhận diện kênh Tiến/Lùi
// Bước 7: Nhận diện kênh Nghiêng, Thao tác:
// ✓ Cần trái: Kéo xuống đáy
// ✓ Cần phải: Đẩy hết sang phải (Nghiêng phải)
// ✓ Giữ trong 3 giây
// -> Hệ thống tự động nhận diện kênh Nghiêng
// Bước 8: Nhận diện kênh Chế độ bay, Thao tác:
// ✓ Trước tiên gạt công tắc mở khóa về vị trí khóa
// ✓ Sau đó gạt công tắc chế độ bay lên mức cao nhất (như chế độ thủ công)
// ✓ Giữ trong 3 giây
// -> Hệ thống tự động nhận diện kênh Chế độ bay

void calibrateRCChannel(float *channel, uint16_t in[16], uint16_t out[16], const char *str) {
	print("%s", str);
	pause(8);
	for (int i = 0; i < 30; i++) readRC(); // try update 30 times max
	memcpy(out, channels, sizeof(channels));

	if (channel == NULL) return; // no channel to calibrate

	// Find channel that changed the most between in and out
	int ch = -1, diff = 0;
	for (int i = 0; i < 16; i++) {
		if (abs(out[i] - in[i]) > diff) {
			ch = i;
			diff = abs(out[i] - in[i]);
		}
	}
	if (ch >= 0 && diff > 10) { // difference threshold is 10
		*channel = ch;
		channelZero[ch] = in[ch];
		channelMax[ch] = out[ch];
	} else {
		*channel = NAN;
	}
}

void printRCCalibration() {
	print("Control   Ch     Zero   Max\n");
	print("Roll      %-7g%-7g%-7g\n", rollChannel, rollChannel >= 0 ? channelZero[(int)rollChannel] : NAN, rollChannel >= 0 ? channelMax[(int)rollChannel] : NAN);
	print("Pitch     %-7g%-7g%-7g\n", pitchChannel, pitchChannel >= 0 ? channelZero[(int)pitchChannel] : NAN, pitchChannel >= 0 ? channelMax[(int)pitchChannel] : NAN);
	print("Yaw       %-7g%-7g%-7g\n", yawChannel, yawChannel >= 0 ? channelZero[(int)yawChannel] : NAN, yawChannel >= 0 ? channelMax[(int)yawChannel] : NAN);
	print("Throttle  %-7g%-7g%-7g\n", throttleChannel, throttleChannel >= 0 ? channelZero[(int)throttleChannel] : NAN, throttleChannel >= 0 ? channelMax[(int)throttleChannel] : NAN);
	print("Mode      %-7g%-7g%-7g\n", modeChannel, modeChannel >= 0 ? channelZero[(int)modeChannel] : NAN, modeChannel >= 0 ? channelMax[(int)modeChannel] : NAN);
}
