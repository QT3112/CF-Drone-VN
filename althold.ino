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
        // Joystick có lò xo (spring-centered): thả ra về vị trí giữa (controlThrottle ≈ 0.5)
        //   - controlThrottle > 0.6  : Bay lên, vận tốc tỷ lệ với mức đẩy
        //   - controlThrottle < 0.4  : Bay xuống, vận tốc tỷ lệ với mức kéo
        //   - 0.4 <= controlThrottle <= 0.6 (vùng chết): Giữ nguyên độ cao
        const float DEAD_HIGH = 0.6f;
        const float DEAD_LOW  = 0.4f;
        const float MAX_CLIMB_SPEED  =  1.0f; // m/s lên
        const float MAX_DESCEND_SPEED = -1.0f; // m/s xuống

        if (controlThrottle > DEAD_HIGH) {
            // Cần ga nâng lên trên vùng chết → Bay lên
            float ratio = (controlThrottle - DEAD_HIGH) / (1.0f - DEAD_HIGH); // 0..1
            targetVelocityZ = ratio * MAX_CLIMB_SPEED;
            targetAltitude  = currentAltitude; // Cập nhật mục tiêu để khóa khi về deadband
        } else if (controlThrottle < DEAD_LOW && controlThrottle > 0.05f) {
            // Cần ga kéo xuống dưới vùng chết → Bay xuống
            float ratio = (DEAD_LOW - controlThrottle) / (DEAD_LOW - 0.05f); // 0..1
            targetVelocityZ = ratio * MAX_DESCEND_SPEED;
            targetAltitude  = currentAltitude;
        } else if (controlThrottle <= 0.05f) {
            // Kéo hết xuống đáy → Hạ cánh / Ngắt động cơ
            targetVelocityZ = -1.5f;
            targetAltitude  = currentAltitude;
        } else {
            // Vùng chết (0.4 → 0.6) → Giữ độ cao hiện tại bằng Position PID
            targetVelocityZ = positionZPID.update(targetAltitude - currentAltitude);
            targetVelocityZ = constrain(targetVelocityZ, -0.5f, 0.5f);
        }

        // Vòng PID Vận tốc Z → Tính lực đẩy bù
        float velocityError = targetVelocityZ - currentVelocityZ;
        float pidOut = velocityZPID.update(velocityError);

        // Mức ga lơ lửng cơ sở (cần tune thực tế)
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
