// Các cài đặt liên quan đến con quay hồi chuyển và gia tốc kế
// Work with the IMU sensor

#include <SPI.h>
#include <FlixPeriph.h>
#include "vector.h"
#include "lpf.h"
#include "util.h"
#include "board_config.h"

MPU9250 imu(SPI, BOARD_SPI_CS);

// Hướng lắp đặt IMU (Góc Euler, rad). Mặc định (0, 0, -PI/2) tương ứng với cách lắp của PCB này:
// Mặt chip hướng lên, in lụa X -> bên phải máy bay, Y -> phía trước máy bay -> Công thức chuyển đổi Vector(data.y, -data.x, data.z)
// Nếu cần thích ứng với hướng lắp đặt khác, hãy sửa giá trị này và chạy lệnh `preset` để reset thông số đã lưu.
Vector imuRotation(0, 0, -PI / 2);

Vector accBias;
Vector accScale(1, 1, 1);
Vector gyroBias;
LowPassFilter<Vector> gyroBiasFilter(0.001); // Bộ lọc thông thấp ước lượng độ lệch của Gyro

void setupIMU() {
	print("Setup IMU\n");
	SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI, BOARD_SPI_CS);
	imu.begin();
	configureIMU();
}

void configureIMU() {
	imu.setAccelRange(imu.ACCEL_RANGE_4G);
	imu.setGyroRange(imu.GYRO_RANGE_2000DPS);
	imu.setDLPF(imu.DLPF_MAX);
	imu.setRate(imu.RATE_1KHZ_APPROX);
	imu.setupInterrupt();
}

void readIMU() {
	imu.waitForData();
	imu.getGyro(gyro.x, gyro.y, gyro.z);
	imu.getAccel(acc.x, acc.y, acc.z);
	calibrateGyroOnce();
	// apply scale and bias
	acc = (acc - accBias) / accScale;
	gyro = gyro - gyroBias;
	// rotate to body frame using imuRotation Euler angles
	Quaternion rotation = Quaternion::fromEuler(imuRotation);
	acc  = Quaternion::rotateVector(acc,  rotation.inversed());
	gyro = Quaternion::rotateVector(gyro, rotation.inversed());
}

void calibrateGyroOnce() {
	static Delay landedDelay(2);
	if (!landedDelay.update(landed)) return; // calibrate only if definitely stationary

	gyroBias = gyroBiasFilter.update(gyro);
}

void calibrateAccel() {
	print("Đang hiệu chuẩn con quay hồi chuyển và gia tốc kế Calibrating accelerometer\n");
	imu.setAccelRange(imu.ACCEL_RANGE_2G); // the most sensitive mode

	print("1/6 Nằm ngang: Đầu hướng về phía trước (hướng bay bình thường), đáy hướng xuống nằm ngang trên bề mặt phẳng, đảm bảo hoàn toàn phẳng không nghiêng. Giữ bất động, sẽ bắt đầu hiệu chuẩn sau 8 giây;\n");
	pause(8);
	calibrateAccelOnce();
	print("Hiệu chuẩn nằm ngang hoàn tất. Vui lòng tiếp tục.\n");
	pause(1);
	print("2/6 Mũi hướng lên: Giữ mũi hướng về phía trước, chĩa mũi lên trời, đuôi chạm mặt phẳng hỗ trợ, vuông góc với mặt đất. Giữ bất động, sẽ bắt đầu hiệu chuẩn sau 8 giây;\n");
	pause(8);
	calibrateAccelOnce();
	print("Hiệu chuẩn mũi hướng lên hoàn tất. Vui lòng tiếp tục.\n");
	pause(1);
	print("3/6 Mũi hướng xuống: Giữ mũi hướng về phía trước, chĩa mũi xuống đất chạm mặt phẳng hỗ trợ, đuôi hướng lên, vuông góc với mặt đất. Giữ bất động, sẽ bắt đầu hiệu chuẩn sau 8 giây;\n");
	pause(8);
	calibrateAccelOnce();
	print("Hiệu chuẩn mũi hướng xuống hoàn tất. Vui lòng tiếp tục.\n");
	pause(1);
	print("4/6 Cạnh phải hướng xuống: Giữ mũi hướng về phía trước, đặt cạnh phải hướng xuống chạm mặt phẳng hỗ trợ, tay đòn bên trái hướng lên, vuông góc với mặt đất. Giữ bất động, sẽ bắt đầu hiệu chuẩn sau 8 giây;\n");
	pause(8);
	calibrateAccelOnce();
	print("Hiệu chuẩn cạnh phải hướng xuống hoàn tất. Vui lòng tiếp tục.\n");
	pause(1);
	print("5/6 Cạnh trái hướng xuống: Giữ mũi hướng về phía trước, đặt cạnh trái hướng xuống chạm mặt phẳng hỗ trợ, tay đòn bên phải hướng lên, vuông góc với mặt đất. Giữ bất động, sẽ bắt đầu hiệu chuẩn sau 8 giây;\n");
	pause(8);
	calibrateAccelOnce();
	print("Hiệu chuẩn cạnh trái hướng xuống hoàn tất. Vui lòng tiếp tục.\n");
	pause(1);
	print("6/6 Lộn ngược: Giữ mũi hướng về phía trước, lật úp máy bay hoàn toàn, đỉnh hướng xuống, đáy hướng lên, toàn bộ nằm ngang úp xuống. Giữ bất động, sẽ bắt đầu hiệu chuẩn sau 8 giây;\n");
	pause(8);
	calibrateAccelOnce();

	printIMUCalibration();
	print("✓ Hoàn tất tất cả các bước hiệu chuẩn! Đặt thân máy bay thẳng lại, chạy lệnh ps để xem kết quả hiệu chuẩn.\n");
	configureIMU();
}

void calibrateAccelOnce() {
	const int samples = 1000;
	static Vector accMax(-INFINITY, -INFINITY, -INFINITY);
	static Vector accMin(INFINITY, INFINITY, INFINITY);

	// Compute the average of the accelerometer readings
	acc = Vector(0, 0, 0);
	for (int i = 0; i < samples; i++) {
		imu.waitForData();
		Vector sample;
		imu.getAccel(sample.x, sample.y, sample.z);
		acc = acc + sample;
	}
	acc = acc / samples;

	// Update the maximum and minimum values
	if (acc.x > accMax.x) accMax.x = acc.x;
	if (acc.y > accMax.y) accMax.y = acc.y;
	if (acc.z > accMax.z) accMax.z = acc.z;
	if (acc.x < accMin.x) accMin.x = acc.x;
	if (acc.y < accMin.y) accMin.y = acc.y;
	if (acc.z < accMin.z) accMin.z = acc.z;
	// Compute scale and bias
	accScale = (accMax - accMin) / 2 / ONE_G;
	accBias = (accMax + accMin) / 2;
}

void printIMUCalibration() {
	print("gyro bias: %f %f %f\n", gyroBias.x, gyroBias.y, gyroBias.z);
	print("accel bias: %f %f %f\n", accBias.x, accBias.y, accBias.z);
	print("accel scale: %f %f %f\n", accScale.x, accScale.y, accScale.z);
}

void printIMUInfo() {
	imu.status() ? print("status: ERROR %d\n", imu.status()) : print("status: OK\n");
	print("model: %s\n", imu.getModel());
	print("who am I: 0x%02X\n", imu.whoAmI());
	print("rate: %.0f\n", loopRate);
	print("gyro: %f %f %f\n", gyro.x, gyro.y, gyro.z);
	print("acc: %f %f %f\n", acc.x, acc.y, acc.z);
	imu.waitForData();
	Vector rawGyro, rawAcc;
	imu.getGyro(rawGyro.x, rawGyro.y, rawGyro.z);
	imu.getAccel(rawAcc.x, rawAcc.y, rawAcc.z);
	print("raw gyro: %f %f %f\n", rawGyro.x, rawGyro.y, rawGyro.z);
	print("raw acc: %f %f %f\n", rawAcc.x, rawAcc.y, rawAcc.z);
}
