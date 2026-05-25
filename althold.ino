// Xử lý cảm biến khoảng cách VL53L1X và thuật toán giữ độ cao (ALTHOLD)

#include <Wire.h>
#include <VL53L1X.h>
#include "board_config.h"
#include "pid.h"
#include "util.h"

VL53L1X sensor;

bool altholdReady = false;
float altholdThrustTarget = 0.0f;
float currentAltitude = 0.0f;  // mét
float targetAltitude = 0.0f;   // mét
float currentVelocityZ = 0.0f; // m/s
float targetVelocityZ = 0.0f;  // m/s

// Thông số PID vận tốc Z (cơ bản)
PID velocityZPID(0.5, 0.1, 0.02, 0.2); // P, I, D, I_LIM (cần tune thực tế)
// Thông số PID vị trí Z (cơ bản)
PID positionZPID(1.0, 0.0, 0.0); 

extern float controlThrottle; // Lấy từ rc.ino
extern int mode;              // Lấy từ control.ino

float lastAltTime = 0;
float lastAlt = 0;

void setupAltHold() {
    print("Setup ALTHOLD (VL53L1X)\n");
    Wire.begin(BOARD_I2C_SDA_PIN, BOARD_I2C_SCL_PIN);
    Wire.setClock(400000); // 400 kHz I2C

    sensor.setTimeout(500);
    if (!sensor.init()) {
        print("VL53L1X: Failed to detect and initialize sensor!\n");
        return;
    }
    
    // Tối ưu cho khoảng cách tầm ngắn/trung, tốc độ cập nhật nhanh
    sensor.setDistanceMode(VL53L1X::Medium);
    sensor.setMeasurementTimingBudget(20000); // 20ms
    sensor.startContinuous(20); // 50Hz update rate
    altholdReady = true;
    print("VL53L1X Initialized for ALTHOLD.\n");
}

void updateAltHold() {
    if (!altholdReady) {
        altholdThrustTarget = controlThrottle; // Fallback về ga tay nếu cảm biến lỗi
        return;
    }

    if (sensor.dataReady()) {
        sensor.read();
        
        float newAlt = sensor.ranging_data.range_mm / 1000.0f; // Đổi sang mét
        
        // Tránh giá trị lỗi từ cảm biến
        if (sensor.ranging_data.range_status == VL53L1X::RangeValid) {
            float now = micros() / 1000000.0f;
            float dt_alt = now - lastAltTime;
            if (dt_alt > 0) {
                currentVelocityZ = (newAlt - lastAlt) / dt_alt;
            }
            currentAltitude = newAlt;
            lastAlt = newAlt;
            lastAltTime = now;
        }
    }

    // Xử lý logic ALTHOLD khi mode == ALTHOLD (3)
    if (mode == 3 /* ALTHOLD */) {
        // Vùng deadband cho cần ga (0.4 đến 0.6 là giữ nguyên độ cao)
        if (controlThrottle > 0.6f) {
            targetVelocityZ = (controlThrottle - 0.6f) * 2.0f; // Tối đa lên 0.8m/s
            targetAltitude = currentAltitude; // Cập nhật lại mục tiêu để khóa khi về deadband
        } else if (controlThrottle < 0.4f && controlThrottle > 0.05f) { // >0.05 để tránh rơi tự do nếu gạt xuống tận cùng khi chưa muốn disarm
            targetVelocityZ = (controlThrottle - 0.4f) * 2.0f; // Tối đa xuống -0.7m/s
            targetAltitude = currentAltitude;
        } else if (controlThrottle <= 0.05f) {
            targetVelocityZ = -1.0f; // Hạ cánh nhanh hoặc idle
            targetAltitude = currentAltitude;
        } else {
            // Cần ga ở giữa (0.4 -> 0.6) -> Giữ vị trí (Velocity = Output của Position PID)
            targetVelocityZ = positionZPID.update(targetAltitude - currentAltitude);
            targetVelocityZ = constrain(targetVelocityZ, -0.5f, 0.5f);
        }

        // Vòng PID Vận tốc Z
        float velocityError = targetVelocityZ - currentVelocityZ;
        float pidOut = velocityZPID.update(velocityError);
        
        // Mức ga lơ lửng cơ sở (cần được tune tay hoặc tự động học, tạm thời fix ở 0.5)
        float hoverThrust = 0.5f; 
        
        altholdThrustTarget = hoverThrust + pidOut;
        altholdThrustTarget = constrain(altholdThrustTarget, 0.05f, 1.0f);
    } else {
        // Reset PID khi không ở chế độ ALTHOLD
        velocityZPID.reset();
        positionZPID.reset();
        targetAltitude = currentAltitude;
        altholdThrustTarget = controlThrottle;
    }
}
