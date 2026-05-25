#pragma once

// Cấu hình phần cứng Board
// Board-level hardware configuration
// Tự động phân biệt ESP32, ESP32-C3 và ESP32-S3 qua chỉ thị biên dịch có điều kiện, không cần sửa thủ công

#ifdef CONFIG_IDF_TARGET_ESP32C3
// ---------------------- ESP32C3 ------------------------- 

// ---- Chân Motor (Thứ tự mảng: RL, RR, FR, FL, giống hằng số trong motors.ino) ----
// GPIO0/1/3/10 đều không có pull-up nội, không có chức năng Strapping (Chân Strapping của C3 chỉ có GPIO2/8/9)
#define BOARD_MOTOR_PINS   {3, 10, 0, 1}    // RL=GPIO3, RR=GPIO10, FR=GPIO0, FL=GPIO1

// ---- Chân ADC Pin ----
// GPIO2 là chân Strapping (có pull-up yếu nội bộ), nhưng chế độ ADC input không bị ảnh hưởng độ chính xác
// Sau khi khởi động, lấy mẫu Strapping hoàn tất, khi gọi analogRead() chân sẽ ở trạng thái input bình thường
#define BOARD_VBAT_ADC_PIN 2               // ADC1_CH2（GPIO2）

// ---- Serial RC (SBUS) ----
// GPIO9 có pull-up SCL I2C trên board + pull-up Boot nội bộ, khi làm SBUS RX input mức cao idle của SBUS sẽ đồng nhất với pull-up
#define BOARD_RC_SERIAL    Serial1
#define BOARD_RC_RX_PIN    9

// ---- SPI IMU (Sử dụng chân mặc định trong tài liệu / Arduino pins_arduino.h) ----
// Hoàn toàn giống định nghĩa SPI mặc định của thư viện Arduino ESP32-C3, có thể gọi SPI.begin() không tham số
#define BOARD_SPI_SCK      4               // Tài liệu SCK  = GPIO4
#define BOARD_SPI_MISO     5               // Tài liệu MISO = GPIO5
#define BOARD_SPI_MOSI     6               // Tài liệu MOSI = GPIO6
#define BOARD_SPI_CS       7               // Tài liệu SS   = GPIO7

// ---- LED: Không có LED trên board, vô hiệu hóa ----
#define BOARD_LED_ENABLED  0

#elif defined(CONFIG_IDF_TARGET_ESP32S3)
// ---------------------- ESP32S3 -------------------------

// ---- Chân Motor: GPIO4-7, xa khu vực FSPI (GPIO10-13), không bị ràng buộc Strapping ----
#define BOARD_MOTOR_PINS   {4, 5, 6, 7}    // RL=GPIO4, RR=GPIO5, FR=GPIO6, FL=GPIO7

// ---- ADC Pin: GPIO1 (ADC1_CH0, dải ADC1 của S3 là GPIO1-10) ----
#define BOARD_VBAT_ADC_PIN 1

// ---- Serial RC (SBUS): GPIO8, không xung đột với FSPI/UART0 ----
#define BOARD_RC_SERIAL    Serial2
#define BOARD_RC_RX_PIN    8

// ---- SPI IMU: Sử dụng các chân mặc định FSPI của ESP32-S3 ----
// SCK=12 (mặc định), MISO=13 (mặc định), MOSI=11 (mặc định), CS=10 (mặc định SS)
#define BOARD_SPI_SCK      12
#define BOARD_SPI_MISO     13
#define BOARD_SPI_MOSI     11
#define BOARD_SPI_CS       10

// ---- LED: PCB mới nối LED thường ở GPIO2, cách điều khiển giống ESP32 ----
#define BOARD_LED_ENABLED  1

#else  // ---- Cấu hình mặc định ESP32 ----
// ---------------------- Mặc định ESP32 -------------------------

#define BOARD_MOTOR_PINS   {12, 13, 15, 14}  // RL=GPIO12, RR=GPIO13, FR=GPIO15, FL=GPIO14
#define BOARD_VBAT_ADC_PIN 36
#define BOARD_RC_SERIAL    Serial2
#define BOARD_RC_RX_PIN    4
#define BOARD_SPI_SCK      18
#define BOARD_SPI_MISO     19
#define BOARD_SPI_MOSI     23
#define BOARD_SPI_CS       5
#define BOARD_LED_ENABLED  1

#endif
