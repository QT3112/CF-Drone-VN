// Máy chủ Web RC

#if WEB_RC_ENABLED

#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "web_rc_html.h"

// Biến điều khiển FC hợp nhất (cho lớp điều hợp giao thức ghi, dùng chung với SBUS/MAVLink)
extern float t;
extern float controlTime;
extern float controlRoll, controlPitch, controlYaw, controlThrottle, controlMode;

// ==================== Hằng số cấu hình ====================
#define WEB_RC_TIMEOUT_MS    10000          // Timeout kết nối: Quá thời gian này (ms) tính từ gói cuối là mất kết nối; phải lớn hơn khoảng heartbeat
// VBAT_ADC_PIN / VBAT_ADC_SAMPLES / VBAT_DIVIDER đã được chuyển sang battery.ino

// ==================== Cờ trạng thái kết nối ====================
bool webRCEnabled    = false;  // Web RC hiện có kết nối hợp lệ (cập nhật mỗi khung hình bởi readWebRC())
bool useWebRC        = false;  // Hiện đang sử dụng điều khiển Web RC (Đồng bộ với webRCEnabled)
bool webRCUpdated    = false;  // Đã nhận được ít nhất 1 lần dữ liệu RC (false trước lần kết nối đầu)
bool webConsoleEnabled = false; // Công tắc Console debug Web: Bật qua POST /console/enable, khi bật print() sẽ ghi vào buffer
static char webRCWarnMsg[64] = ""; // Tin nhắn cảnh báo chờ gửi về frontend, gửi một lần rồi tự xóa

// ==================== Giá trị tạm thời của Cần (Cho endpoint trạng thái đọc)====================
// Đơn vị: Ga 0~100 (%), Trục nghiêng ±30 (°), Nhấn=1 / Thả=0
float webRCRoll     = 0.0f;
float webRCPitch    = 0.0f;
float webRCYaw      = 0.0f;
float webRCThrottle = 0.0f;
uint16_t webRCButtons    = 0;       // Mask nút bấm 16-bit, bit0=mở_khóa bit1=khóa bit2=dừng_khẩn bit6=STAB bit7=ACRO bit8=ALTHOLD
unsigned long webRCLastUpdate = 0;  // Timestamp millis() của gói cuối nhận được

// ==================== Độ nhạy (Scale) ====================
// Đầu ra cuối = Góc đã xử lý × scale / STICK_MAX, kết quả ghi vào control* ([-1,1])
// Giảm scale -> Máy bay phản ứng mượt hơn; Tăng -> Nhạy hơn
float webRCThrottleScale = 1.0f;    // Giới hạn công suất tối đa của Ga, 1.0=100%, 0.8=Chỉ đẩy lên được 80% Ga
float webRCStickScale    = 0.85f;   // Độ nhạy Roll/Pitch, đề xuất 0.5~1.0
float webRCYawScale      = 0.68f;   // Độ nhạy Yaw, do quán tính Yaw nhỏ nên để thấp, đề xuất 0.3~0.8

// ==================== Vùng chết (Deadzone) ====================
// Không gian chuẩn hóa (tỷ lệ 0~1), lỗi cần ở giữa nằm trong vùng chết sẽ output 0
// Tăng -> Vùng giữa rộng hơn, ít chạm nhầm; Giảm -> Phản hồi nhạy hơn nhưng dễ trôi
float stickDeadzone    = 0.06f;     // Roll/Pitch/Yaw dùng chung, vùng chết chuẩn hóa 6%
float throttleDeadzone = 0.06f;     // Vùng chết cực dưới của Ga, đẩy dưới mức này sẽ output 0% (Chống nhiễu)

// ==================== Khoảng giá trị đầu vào (Đừng tùy tiện sửa, cần đồng bộ với Frontend)====================
static const float THROTTLE_MIN = 0.0f;    // Giới hạn dưới đầu ra Ga (%)
static const float THROTTLE_MAX = 100.0f;  // Giới hạn trên đầu ra Ga (%)
static const float STICK_MAX    = 30.0f;   // Góc trục tối đa (°), tương ứng kéo hết mức ở Frontend; Sửa cái này thì phải đổi PID TILT_MAX theo
static const float RAW_MAX      = 100.0f;  // Mức chuẩn hóa kéo hết của Frontend, JS ở Frontend fix output ±100

// ==================== Trạng thái nội bộ (Khi chạy, ĐỪNG tự ý sửa)====================
static float lastValidThrottle = 0.0f;     // Giá trị Ga đã xác thực lần trước, để dùng lại nếu có lỗi
static float lastValidRoll     = 0.0f;     // Giá trị Roll đã xác thực lần trước
static float lastValidPitch    = 0.0f;     // Giá trị Pitch đã xác thực lần trước
static float lastValidYaw      = 0.0f;     // Giá trị Yaw đã xác thực lần trước
static unsigned long lastDataErrorTime = 0; // Thời điểm bị lỗi data lần trước, dùng để limit log báo lỗi

// ==================== Server Web ====================
WebServer webRCServer(8080);

// ==================== Buffer log Console ====================
#define CONSOLE_LINES    50
#define CONSOLE_LINE_LEN 240
static char consoleBuf[CONSOLE_LINES][CONSOLE_LINE_LEN];
static int  consoleTail   = 0;
static int  consoleFilled = 0;
static int  consoleTotal  = 0;   // Tổng số dòng tăng đơn điệu, dùng để fetch lần lượt (increment pull)

#define CONSOLE_CMD_QUEUE_SIZE 4
#define CONSOLE_CMD_LEN 64
static char consoleCmdQueue[CONSOLE_CMD_QUEUE_SIZE][CONSOLE_CMD_LEN];
static int  consoleCmdHead  = 0;
static int  consoleCmdTail  = 0;
static int  consoleCmdCount = 0;

bool enqueueConsoleCmd(const char* cmd) {
    if (!cmd || !*cmd) return false;
    if (consoleCmdCount >= CONSOLE_CMD_QUEUE_SIZE) return false;
    strncpy(consoleCmdQueue[consoleCmdTail], cmd, CONSOLE_CMD_LEN - 1);
    consoleCmdQueue[consoleCmdTail][CONSOLE_CMD_LEN - 1] = '\0';
    consoleCmdTail = (consoleCmdTail + 1) % CONSOLE_CMD_QUEUE_SIZE;
    consoleCmdCount++;
    return true;
}

void processConsoleCommandQueue() {
    if (consoleCmdCount <= 0) return;

    String cmd = consoleCmdQueue[consoleCmdHead];
    consoleCmdHead = (consoleCmdHead + 1) % CONSOLE_CMD_QUEUE_SIZE;
    consoleCmdCount--;
    doCommand(cmd, false);
}

void webLog(const char* msg) {
    // Split bằng \n khi viết, tránh dấu ngắt dòng làm hỏng cấu trúc JSON
    const char* start = msg;
    while (*start) {
        const char* end = strchr(start, '\n');
        int len = end ? (int)(end - start) : (int)strlen(start);
        if (len > 0) {
            int copy = (len < CONSOLE_LINE_LEN - 1) ? len : (CONSOLE_LINE_LEN - 1);
            strncpy(consoleBuf[consoleTail], start, copy);
            consoleBuf[consoleTail][copy] = '\0';
            consoleTail = (consoleTail + 1) % CONSOLE_LINES;
            if (consoleFilled < CONSOLE_LINES) consoleFilled++;
            consoleTotal++;
        }
        if (!end) break;
        start = end + 1;
    }
}

// ==================== Xử lý tay gạt (RC) ====================

// Vùng chết (Chuẩn hóa, input/output đều là [-1, 1])
// Phần ngoài vùng chết sẽ được map tuyến tính tới mức cao nhất, kéo hết tay = đẩy hết mức
float applyDeadzone(float norm, float deadzone) {
    if (fabsf(norm) < deadzone) return 0.0f;
    float sign = (norm > 0.0f) ? 1.0f : -1.0f;
    return sign * (fabsf(norm) - deadzone) / (1.0f - deadzone);
}

// Xử lý trục cân bằng (Roll/Pitch/Yaw)
// Input raw thuộc [-100, +100], output thuộc [-STICK_MAX, +STICK_MAX] (°)
float processAxis(float raw, float& lastValid) {
    if (isnan(raw) || isinf(raw) || fabsf(raw) > 1000.0f) {
        if (millis() - lastDataErrorTime > 2000) lastDataErrorTime = millis();
        return lastValid;
    }
    float norm = constrain(raw, -RAW_MAX, RAW_MAX) / RAW_MAX;
    norm = applyDeadzone(norm, stickDeadzone);
    lastValid = norm * STICK_MAX;
    return lastValid;
}

// Xử lý trục Ga
// Frontend: Trục Y trái, Đáy=-100, Đỉnh=+100
// Output: pct thuộc [0, 100]
float processThrottle(float raw) {
    if (isnan(raw) || isinf(raw) || fabsf(raw) > 1000.0f) return lastValidThrottle;
    raw = constrain(raw, -RAW_MAX, RAW_MAX);
    // Mapping tuyến tính: -100->0%, 0->50%, +100->100%
    float pct = (raw + RAW_MAX) / (2.0f * RAW_MAX) * THROTTLE_MAX;
    if (pct < throttleDeadzone * THROTTLE_MAX) pct = 0.0f;
    pct = constrain(pct * webRCThrottleScale, THROTTLE_MIN, THROTTLE_MAX);
    lastValidThrottle = pct;
    return pct;
}

// ==================== Xử lý dữ liệu lõi ====================

void setWebRCInput(float roll, float pitch, float yaw, float throttle, uint16_t buttons) {
    float pThrottle = processThrottle(throttle);
    float pYaw      = processAxis(yaw,   lastValidYaw);
    float pPitch    = processAxis(pitch, lastValidPitch);
    float pRoll     = processAxis(roll,  lastValidRoll);

    // Lưu tạm giá trị đã xử lý (Cho endpoint trạng thái đọc)
    webRCThrottle = pThrottle;
    webRCYaw      = pYaw;
    webRCPitch    = pPitch;
    webRCRoll     = pRoll;
    webRCButtons  = buttons;
    webRCLastUpdate = millis();
    webRCUpdated  = true;

    // Viết vào các biến điều khiển toàn cục (Chung với SBUS/MAVLink)
    controlRoll     = constrain(pRoll  * webRCStickScale / STICK_MAX, -1.0f, 1.0f);
    controlPitch    = constrain(pPitch * webRCStickScale / STICK_MAX, -1.0f, 1.0f);
    controlYaw      = constrain(pYaw   * webRCYawScale   / STICK_MAX, -1.0f, 1.0f);
    controlThrottle = pThrottle / THROTTLE_MAX;
    controlMode     = NAN;
    controlTime     = t;

    static float lastPrintedThrottle = -1.0f;
    if (fabsf(pThrottle - lastPrintedThrottle) > 5.0f) {
        print("WebRC T=%.0f%% R=%.1f P=%.1f Y=%.1f Btn=0x%04X\n",
              pThrottle, pRoll, pPitch, pYaw, buttons);
        lastPrintedThrottle = pThrottle;
    }
}

// ==================== Trình xử lý giao thức JSON ====================

const char* findJsonValue(const char* json, const char* key) {
    const char* p = strstr(json, key);
    if (!p) return nullptr;
    p = strchr(p, ':');
    if (!p) return nullptr;
    p++;
    while (*p == ' ' || *p == '"') p++;
    return p;
}

bool handleJSONProtocol(String& body) {
    if (body.length() == 0 || body.indexOf('{') == -1) return false;
    const char* json = body.c_str();
    const char* typePos = strstr(json, "\"t\":");
    if (!typePos) return false;
    int type = atoi(typePos + 4);
    const char* v;

    switch (type) {
        case 1: { // Dữ liệu Remote RC
            float th = 0, r = 0, p = 0, y = 0;
            uint32_t ts = 0;
            if ((v = findJsonValue(json, "\"th\""))) th = atof(v);
            if ((v = findJsonValue(json, "\"r\"")))  r  = atof(v);
            if ((v = findJsonValue(json, "\"p\"")))  p  = atof(v);
            if ((v = findJsonValue(json, "\"y\"")))  y  = atof(v);
            if ((v = findJsonValue(json, "\"ts\""))) ts = atol(v);
            setWebRCInput(r, p, y, th, webRCButtons);
            break;
        }
        case 2: { // Sự kiện nút nhấn
            int idx = 0, state = 0;
            uint32_t ts = 0;
            if ((v = findJsonValue(json, "\"b\"")))  idx   = atoi(v);
            if ((v = findJsonValue(json, "\"s\"")))  state = atoi(v);
            if ((v = findJsonValue(json, "\"ts\""))) ts    = atol(v);
            if (idx >= 0 && idx < 16) {
                if (state) webRCButtons |=  (1 << idx);
                else       webRCButtons &= ~(1 << idx);
                webRCLastUpdate = millis();
                webRCUpdated    = true;
            }
            break;
        }
        case 4: // Gói Heartbeat
            webRCLastUpdate = millis();
            webRCUpdated    = true;
            break;
    }
    return true;
}

void setWebRCWarn(const char* msg) {
    strncpy(webRCWarnMsg, msg, sizeof(webRCWarnMsg) - 1);
    webRCWarnMsg[sizeof(webRCWarnMsg) - 1] = '\0';
}

// ==================== Xử lý HTTP Request ====================

void handleWebRCRequest() {
    if (!webRCServer.hasArg("plain")) {
        webRCServer.send(400, "application/json", "{\"e\":\"no data\"}");
        return;
    }
    String body = webRCServer.arg("plain");
    if (handleJSONProtocol(body)) {
        char resp[256];
        if (webRCWarnMsg[0]) {
            snprintf(resp, sizeof(resp),
                "{\"s\":\"ok\",\"m\":%d,\"arm\":%d,\"warn\":\"%s\"}",
                mode, (int)armed, webRCWarnMsg);
            webRCWarnMsg[0] = '\0'; // Gửi xong sẽ xóa rỗng ngay
        } else {
            snprintf(resp, sizeof(resp),
                "{\"s\":\"ok\",\"m\":%d,\"arm\":%d}",
                mode, (int)armed);
        }
        webRCServer.send(200, "application/json", resp);
    } else {
        webRCServer.send(400, "application/json", "{\"e\":\"parse failed\"}");
    }
}

// ==================== Điện áp pin ====================
// readBatteryVoltage() đã chuyển sang battery.ino, gọi trực tiếp là được

// ==================== Trạng thái kết nối ====================

bool isWebRCEnabled() {
    return webRCUpdated && (millis() - webRCLastUpdate < WEB_RC_TIMEOUT_MS);
}

bool isUsingWebRC() {
    return useWebRC && isWebRCEnabled();
}

// ==================== Hàm khởi tạo chính (setup) ====================

void setupWebRC() {
    lastValidThrottle = THROTTLE_MIN;
    lastValidRoll = lastValidPitch = lastValidYaw = 0.0f;

    webRCServer.on("/", HTTP_GET, []() {
        webRCServer.send(200, "text/html", webRCIndexHtml);
    });
    webRCServer.on("/web_rc",           HTTP_POST, handleWebRCRequest);
    webRCServer.on("/web_rc/heartbeat", HTTP_POST, handleWebRCRequest);

    webRCServer.on("/console", HTTP_GET, []() {
        int since = -1;
        int limit = 20;
        if (webRCServer.hasArg("since")) since = webRCServer.arg("since").toInt();
        if (webRCServer.hasArg("limit")) limit = webRCServer.arg("limit").toInt();
        if (limit <= 0) limit = 20;
        if (limit > CONSOLE_LINES) limit = CONSOLE_LINES;

        int availableFrom = max(0, consoleTotal - consoleFilled);
        int sendFrom = (since >= 0) ? since : availableFrom;
        if (sendFrom < availableFrom) sendFrom = availableFrom;
        if (sendFrom > consoleTotal) sendFrom = consoleTotal;
        int sendTo = min(consoleTotal, sendFrom + limit);

        String json = "{\"total\":";
        json += consoleTotal;
        json += ",\"next\":";
        json += sendTo;
        json += ",\"has_more\":";
        json += (sendTo < consoleTotal) ? "true" : "false";
        json += ",\"lines\":[";

        bool first = true;
        for (int i = sendFrom; i < sendTo; i++) {
            int idx = i % CONSOLE_LINES;
            if (!first) json += ",";
            first = false;
            json += "\"";
            String line = consoleBuf[idx];
            line.replace("\\", "\\\\");  // \ → \\
            line.replace("\"", "\\\"");  // " → \"
            line.replace("\n", "\\n");    // Dự phòng: Dòng mới -> \n
            line.replace("\r", "\\r");    // Dự phòng: CR -> \r
            json += line + "\"";
        }
        json += "]}";
        webRCServer.send(200, "application/json", json);
    });

    webRCServer.on("/console/cmd", HTTP_POST, []() {
        String cmd = webRCServer.arg("plain");
        cmd.trim();
        if (cmd.length() == 0) {
            webRCServer.send(400, "application/json", "{\"ok\":0,\"e\":\"empty command\"}");
            return;
        }

        char buf[CONSOLE_LINE_LEN];
        snprintf(buf, sizeof(buf), "> %s", cmd.c_str());
        webLog(buf);

        if (!enqueueConsoleCmd(cmd.c_str())) {
            webLog("! command queue is full");
            webRCServer.send(503, "application/json", "{\"ok\":0,\"e\":\"queue full\"}");
            return;
        }

        webRCServer.send(200, "application/json", "{\"ok\":1,\"queued\":1}");
    });

    webRCServer.on("/console/enable", HTTP_POST, []() {
        webConsoleEnabled = true;
        webRCServer.send(200, "application/json", "{\"ok\":1}");
    });

    webRCServer.on("/console/disable", HTTP_POST, []() {
        webConsoleEnabled = false;
        webRCServer.send(200, "application/json", "{\"ok\":1}");
    });

    webRCServer.on("/web_rc/status", HTTP_GET, []() {
        float vbat = readBatteryVoltage();
        if (isnan(vbat) || vbat < 0.0f) vbat = 0.0f;
        char json[384];
        snprintf(json, sizeof(json),
            "{\"enabled\":%s,\"active\":%s,"
            "\"voltage\":%.2f,"
            "\"throttle\":%.1f,\"roll\":%.1f,\"pitch\":%.1f,\"yaw\":%.1f}",
            isWebRCEnabled() ? "true" : "false",
            isUsingWebRC()   ? "true" : "false",
            vbat,
            webRCThrottle, webRCRoll, webRCPitch, webRCYaw);
        webRCServer.send(200, "application/json", json);
    });

    webRCServer.begin();
    print("✓ Đã khởi động Web RC: http://192.168.4.1:8080\n");
    print("  Vùng chết Cần=%.0f%% Ga=%.0f%% | Tỉ lệ Cần=%.2f Xoay=%.2f\n",
          stickDeadzone * 100.0f, throttleDeadzone * 100.0f,
          webRCStickScale, webRCYawScale);
}

// ==================== Hàm vòng lặp chính ====================

void readWebRC() {
    webRCServer.handleClient();
    if (isWebRCEnabled()) {
        webRCEnabled = useWebRC = true;
    } else {
        webRCEnabled = useWebRC = false;
    }
}

#else
void setupWebRC() { print("Web RC đã tắt\n"); }
void readWebRC()  {}
void processConsoleCommandQueue() {}
#endif
