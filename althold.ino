// Giữ độ cao (ALTHOLD) — Chế độ bay DUY NHẤT
//
// State machine:
//   AH_IDLE     → Drone đã arm, motor KHÔNG quay. Chờ cử chỉ PRE-SPIN.
//   AH_PRESPIN  → Motor quay nhẹ (motThrMin), chưa bay. Chờ Joystick trái lên.
//   AH_FLYING   → ALTHOLD PID hoạt động, drone giữ/thay đổi độ cao.
//
// Cử chỉ PRE-SPIN: Gạt Joystick PHẢI lên > 80% rồi thả về < 20%.
// Tự động DISARM: Altitude < 10cm + thrust thấp trong 1.5s khi đang bay.

#include <Wire.h>
#include <VL53L1X.h>
#include "board_config.h"
#include "pid.h"
#include "util.h"

// ---- Cảm biến ----
static VL53L1X sensor;
bool  altholdReady    = false;
float currentAltitude = 0.0f;  // Độ cao hiện tại (mét)
float currentVelocityZ = 0.0f; // Vận tốc thẳng đứng (m/s)

// ---- Ngõ ra cho control.ino ----
float altholdThrustTarget = 0.0f; // Lực đẩy yêu cầu [0..1]

// ---- Tham số tuning (có thể chỉnh qua 'p AH_HOVER_THR ...' ) ----
float hoverThrust   = 0.50f; // Mức ga lơ lửng cơ sở, tăng nếu drone không bay được
float maxClimbSpeed = 1.0f;  // Tốc độ bay lên tối đa (m/s)

// ---- PID vận tốc Z và vị trí Z ----
static PID velocityZPID(0.5f, 0.1f, 0.02f, 0.2f);
static PID positionZPID(1.0f, 0.0f, 0.0f);

// ---- Extern cần dùng ----
extern float controlThrottle; // Joystick TRÁI dọc (0..1, giữa = 0.5)
extern float controlPitch;    // Joystick PHẢI dọc (-1..1, lên = +1)
extern float motThrMin;       // Mức ga tối thiểu để motor quay
extern bool  armed;
extern float t;

// ---- Biến nội bộ ----
static float targetAltitude  = 0.0f;
static float targetVelocityZ = 0.0f;
static float lastAltTime     = 0.0f;
static float lastAlt         = 0.0f;

// ---- Trạng thái ----
typedef enum { AH_IDLE = 0, AH_PRESPIN = 1, AH_FLYING = 2 } AltHoldState;
AltHoldState ahState = AH_IDLE;

// ---- Ngưỡng điều khiển ----
static const float THROTTLE_DEAD_HIGH  = 0.60f; // JS trái > 60%  → bay lên
static const float THROTTLE_DEAD_LOW   = 0.40f; // JS trái < 40%  → hạ xuống
static const float MAX_DESCEND_SPEED   = 1.0f;  // m/s tối đa hạ xuống
static const float PRESPIN_PITCH_ON    = 0.80f; // JS phải lên > 80% → nhận cử chỉ
static const float PRESPIN_PITCH_OFF   = 0.20f; // JS phải về  < 20% → xác nhận
static const float LAND_ALT_THRESHOLD  = 0.10f; // Dưới 10cm → xem như đang đáp
static const float LAND_THRUST_FACTOR  = 1.2f;  // Thrust < motThrMin*1.2 → đang đáp
static const float LAND_CONFIRM_TIME   = 1.5f;  // Giữ điều kiện 1.5s → tự disarm

// Mức ga PRE-SPIN: phải đủ để motor quay nghe thấy tiếng, nhưng KHÔNG nhấc bổng
// Mặc định = motThrMin + 0.08 (~18%). Chỉnh qua AH_PRESPIN_THR nếu cần
float preSpinThrust = 0.18f; // Có thể chỉnh qua 'p AH_PRESPIN_THR ...'

// ---------------------------------------------------------------------------
// Khởi tạo
// ---------------------------------------------------------------------------
void setupAltHold() {
    print("Setup ALTHOLD (VL53L1X)\n");
    Wire.begin(BOARD_I2C_SDA_PIN, BOARD_I2C_SCL_PIN);
    Wire.setClock(400000);

    sensor.setTimeout(500);
    if (!sensor.init()) {
        print("VL53L1X: Không tìm thấy cảm biến! Sẽ dùng ga trực tiếp làm fallback.\n");
        return;
    }
    sensor.setDistanceMode(VL53L1X::Medium);
    sensor.setMeasurementTimingBudget(20000); // 20ms
    sensor.startContinuous(20);              // 50Hz
    altholdReady = true;
    print("VL53L1X: Khoi tao OK\n");
}

// ---------------------------------------------------------------------------
// Đọc cảm biến (gọi mỗi vòng lặp)
// ---------------------------------------------------------------------------
static void readAltSensor() {
    if (!altholdReady) return;
    if (!sensor.dataReady()) return;
    sensor.read();
    if (sensor.ranging_data.range_status != VL53L1X::RangeValid) return;

    float newAlt = sensor.ranging_data.range_mm / 1000.0f;
    float now    = micros() / 1000000.0f;
    float dtAlt  = now - lastAltTime;

    if (dtAlt > 0.001f && dtAlt < 0.5f) {
        currentVelocityZ = (newAlt - lastAlt) / dtAlt;
    }
    currentAltitude = newAlt;
    lastAlt         = newAlt;
    lastAltTime     = now;
}

// ---------------------------------------------------------------------------
// Phát hiện cử chỉ PRE-SPIN: JS phải lên > 80% → thả về < 20%
// ---------------------------------------------------------------------------
static bool detectPreSpinGesture() {
    static bool pitchWasHigh = false;

    if (controlPitch > PRESPIN_PITCH_ON) {
        pitchWasHigh = true;
    }
    if (pitchWasHigh && controlPitch < PRESPIN_PITCH_OFF) {
        pitchWasHigh = false;
        return true; // Cử chỉ hoàn chỉnh
    }
    return false;
}

// ---------------------------------------------------------------------------
// Phát hiện hạ cánh (dùng hysteresis thời gian để tránh báo nhầm)
// ---------------------------------------------------------------------------
static bool detectLanding() {
    static float landStartTime = 0.0f;

    bool onGround = (currentAltitude < LAND_ALT_THRESHOLD &&
                     altholdThrustTarget < motThrMin * LAND_THRUST_FACTOR);
    if (onGround) {
        if (landStartTime == 0.0f) landStartTime = t;
        if (t - landStartTime > LAND_CONFIRM_TIME) {
            landStartTime = 0.0f;
            return true;
        }
    } else {
        landStartTime = 0.0f;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Vòng lặp chính ALTHOLD — gọi mỗi chu kỳ từ loop()
// ---------------------------------------------------------------------------
void updateAltHold() {
    readAltSensor();

    // Nếu không có cảm biến: dùng ga trực tiếp làm fallback, không có state machine
    if (!altholdReady) {
        altholdThrustTarget = controlThrottle;
        return;
    }

    // Khi disarm → reset về IDLE
    if (!armed) {
        if (ahState != AH_IDLE) {
            ahState = AH_IDLE;
            velocityZPID.reset();
            positionZPID.reset();
            print("ALTHOLD: IDLE (disarmed)\n");
        }
        altholdThrustTarget = 0.0f;
        return;
    }

    // ---- State Machine ----
    switch (ahState) {

    // B1: Arm xong, motor chưa quay, chờ cử chỉ PRE-SPIN
    case AH_IDLE:
        altholdThrustTarget = 0.0f;
        if (detectPreSpinGesture()) {
            ahState = AH_PRESPIN;
            print("ALTHOLD: PRE-SPIN - Motor quay nhe, chua bay\n");
        }
        break;

    // B2: Motor quay nhẹ, chờ joystick trái lên để cất cánh
    case AH_PRESPIN:
        altholdThrustTarget = preSpinThrust; // Đủ để motor quay nghe thấy, không nhấc bổng
        if (controlThrottle > THROTTLE_DEAD_HIGH) {
            ahState        = AH_FLYING;
            targetAltitude = currentAltitude; // Khóa độ cao ban đầu
            velocityZPID.reset();
            positionZPID.reset();
            print("ALTHOLD: FLYING - Bat dau cat canh\n");
        }
        break;

    // B3: Đang bay — PID giữ độ cao
    case AH_FLYING: {
        // Joystick trái dọc → lệnh vận tốc
        if (controlThrottle > THROTTLE_DEAD_HIGH) {
            float ratio  = (controlThrottle - THROTTLE_DEAD_HIGH) / (1.0f - THROTTLE_DEAD_HIGH);
            targetVelocityZ = ratio * maxClimbSpeed;
            targetAltitude  = currentAltitude; // Cập nhật để khóa khi về deadband
        } else if (controlThrottle < THROTTLE_DEAD_LOW) {
            float ratio  = (THROTTLE_DEAD_LOW - controlThrottle) / THROTTLE_DEAD_LOW;
            targetVelocityZ = -ratio * MAX_DESCEND_SPEED;
            targetAltitude  = currentAltitude;
        } else {
            // Vùng chết → Giữ nguyên độ cao bằng Position PID
            targetVelocityZ = positionZPID.update(targetAltitude - currentAltitude);
            targetVelocityZ = constrain(targetVelocityZ, -0.5f, 0.5f);
        }

        // Velocity PID → thrust
        float velError = targetVelocityZ - currentVelocityZ;
        float pidOut   = velocityZPID.update(velError);
        altholdThrustTarget = hoverThrust + pidOut;
        altholdThrustTarget = constrain(altholdThrustTarget, 0.05f, 1.0f);

        // Phát hiện hạ cánh → tự disarm
        if (detectLanding()) {
            print("ALTHOLD: Phat hien ha canh, tu dong disarm\n");
            armed = false; // Vòng lặp tiếp theo sẽ reset về AH_IDLE
        }
        break;
    }
    } // end switch
}
