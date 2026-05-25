// control.h
#ifndef CONTROL_H
#define CONTROL_H

// Định nghĩa chế độ bay
enum FlightMode {
    MODE_MANUAL = 0,
    MODE_ACRO = 1,
    MODE_STAB = 2,
    MODE_ALTHOLD = 3,  // Giữ độ cao bằng áp kế (Ga = Tốc độ lên xuống)
    MODE_AUTO = 4
};

// Cấu trúc trạng thái điều khiển
struct ControlState {
    int mode;
    bool armed;
    float thrustTarget;
    float rollTarget;
    float pitchTarget;
    float yawTarget;
};

// Khai báo hàm
void control();
void interpretControls();
void interpretWebRC();
void combineInputs();
void controlAttitude();
void controlRates();
void controlTorque();

// Hàm phụ trợ
const char* getModeName(int mode);

// Khai báo hàm bên ngoài
bool isUsingWebRC();
void setWebRCWarn(const char* msg);

#endif // CONTROL_H
