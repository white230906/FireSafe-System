// ======== BLYNK DEVICE CONFIG ========
#include "secrets.h"

#define BLYNK_TEMPLATE_ID "TMPL6u6Ah-ML6"
#define BLYNK_TEMPLATE_NAME "Fire Detection System"
#define BLYNK_PRINT Serial
//thằng này giúp hiển thị thông tin debug trên Serial Monitor,
//giúp lập trình viên theo dõi trạng thái của thiết bị và các thông báo từ Blynk.

#include <Arduino.h>
//thư viện cung cấp các lệnh: digitalRead(), digitalWrite(), delay(): để đọc cảm biến và điều khiển các thiết bị 
#include <Wire.h>
//thư viện giúp ESP32 gửi dữ liệu đến màn hình LCD quan SDA và SCL
#include <ESP32Servo.h>
//thư viện điều khiển servo
#include <WiFi.h>
//thư viện ESP32 kết nối với mạng wifi
#include <BlynkSimpleEsp32.h>
//thư viện giúp ESP32 kết nối với Blynk Cloud
// ============ PIN DEFINITIONS ============


// Servo & Button
#define SERVO_PIN 32
#define BUTTON_PIN 33

// ============ CONFIGURATION ============

// Adjustable by Blynk sliders V4 and V5
float TEMP_WARNING = 40.0;
float TEMP_DANGER = 45.0;

// ESP32 connects to a 2.4 GHz Wi-Fi network. (gồm tên mạng và mật khẩu)
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASSWORD;

const unsigned long CLOUD_SEND_INTERVAL = 2000;
const unsigned long CLOUD_RETRY_INTERVAL = 5000;
const unsigned long WIFI_RETRY_INTERVAL = 15000;
unsigned long lastCloudSend = 0;
unsigned long lastCloudAttempt = 0;
unsigned long lastWiFiAttempt = 0;
bool firstCloudAttempt = true;

// A remote door request is applied AFTER fresh sensor readings.
bool remoteDoorCommandPending = false;
bool remoteDoorOpenRequested = false;
bool doorStateDirty = true;

const int FLAME_ACTIVE = LOW;
const int MQ2_ACTIVE = LOW;

// Door angles
const int DOOR_CLOSE_ANGLE = 0;
const int DOOR_OPEN_ANGLE = 35;
Servo doorServo;
bool doorOpen = false;

// Button debounce
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Warning buzzer: 500ms ON, 500ms OFF
const unsigned long BUZZER_INTERVAL = 500;

// LCD configuration
uint8_t lcdAddress = 0x27;
bool lcdAvailable = false;

#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BL 0x08

// Function prototypes (also compatible with Arduino's auto-prototypes).
void setDoor(bool open);
void updateDoor(bool danger);

// ============ LCD FUNCTIONS ============

bool checkI2C(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}
//kiểm tra LCD có kết nối hay không.
void lcdExpanderWrite(uint8_t data) {
  Wire.beginTransmission(lcdAddress);
  Wire.write(data | LCD_BL);
  Wire.endTransmission();
}
//gửi dữ liệu từ ESP32 đến module I2C
void lcdPulse(uint8_t data) {
  lcdExpanderWrite(data | LCD_EN);
  delayMicroseconds(1);

  lcdExpanderWrite(data & ~LCD_EN);
  delayMicroseconds(50);
}
//tạo tín hiệu để LCD nhận dữ liệu
void lcdWrite4Bits(uint8_t data, uint8_t mode) {
  lcdPulse((data & 0xF0) | mode);
}
// gửi mỗi lần 4 bit dữ liệu.
void lcdSend(uint8_t data, uint8_t mode) {
  lcdWrite4Bits(data & 0xF0, mode);
  lcdWrite4Bits((data << 4) & 0xF0, mode);
}
//chia dữ liệu 8 bit thành hai lần gửi 4 bit.
void lcdCommand(uint8_t command) {
  lcdSend(command, 0);

  if (command == 0x01 || command == 0x02) {
    delay(2);
  }
}
//gửi lệnh điều khiển LCD.
void lcdWriteChar(char character) {
  lcdSend((uint8_t)character, LCD_RS);
}
//gửi một ký tự để hiển thị.
void lcdInitialize() {
  delay(50);

  lcdPulse(0x30);
  delay(5);

  lcdPulse(0x30);
  delayMicroseconds(150);

  lcdPulse(0x30);
  delayMicroseconds(150);

  lcdPulse(0x20);

  lcdCommand(0x28);
  lcdCommand(0x0C);
  lcdCommand(0x06);
  lcdCommand(0x01);
}
//khởi động và cấu hình LCD
void lcdPrintLine(int row, String message) {
  if (!lcdAvailable) return;

  lcdCommand(row == 0 ? 0x80 : 0xC0);

  for (int i = 0; i < 16; i++) {
    if (i < (int)message.length()) {
      lcdWriteChar(message[i]);
    } else {
      lcdWriteChar(' ');
    }
  }
}
//in nội dung lên dòng 1 hoặc dòng 2, tối đa 16 ký tự.
// ============ READ TEMPERATURE ============

float readTemperature() {
  uint32_t total = 0;

  for (int i = 0; i < 10; i++) {
    total += analogReadMilliVolts(LM35_PIN);
    delay(5);
  }

  float voltageMv = total / 10.0;

  // LM35: 10mV = 1 degree Celsius
  return voltageMv / 10.0;
}

// ============ OUTPUT CONTROL ============

void controlOutputs(bool danger, float temperature) {

  // Turn off LEDs
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  if (danger) {

    // DANGER: Red LED + continuous buzzer
    digitalWrite(LED_RED, HIGH);

    digitalWrite(BUZZER_PIN, HIGH);
  }
  else if (temperature >= TEMP_WARNING) {

    // WARNING: Yellow LED
    digitalWrite(LED_YELLOW, HIGH);

    // Intermittent buzzer using millis()
    bool buzzerState =
        (millis() / BUZZER_INTERVAL) % 2 == 0;
      //cứ mỗi 500ms thì buzzer chuyển trạng thái bậc và tắt
    digitalWrite(BUZZER_PIN,
                 buzzerState ? HIGH : LOW);
  }
  else {

    // SAFE: Green LED, buzzer OFF
    digitalWrite(LED_GREEN, HIGH);

    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ============ BLYNK DATASTREAM HANDLERS ============
// V0: Temperature (out)  ESP32 gửi nhiệt độ lên Blynk
// V1: Flame Detected (out)  gửi trạng thái phát hiện lửa
// V2: Smoke Detected (out)  gửi trạng thái MQ-2
// V3: System Status (out)   gửi SAFE/WARNING/DANGER
// V4: Warning Threshold (in) nhận ngưỡng Warning từ Blynk
// V5: Danger Threshold (in) nhận ngưỡng Danger từ Blynk
// V6: Door Status (out) gửi trạng thái cửa
// V7: Door Control (in + state feedback) nhận lệnh đóng/mở và phản hồi trạng thái cửa

BLYNK_WRITE(V4) {//nhận ngưỡng Warning từ Blynk
  int value = param.asInt();
  if (value >= 30 && value <= 60 && value < TEMP_DANGER) {
    TEMP_WARNING = value;
    Serial.printf("Blynk WARNING threshold: %d C\n", value);
  } else {
    // Reject an invalid setting and restore the Slider display.
    Blynk.virtualWrite(V4, (int)TEMP_WARNING);
  }
}

BLYNK_WRITE(V5) { //nhận ngưỡng Danger từ Blynk
  int value = param.asInt();
  if (value >= 35 && value <= 90 && value > TEMP_WARNING) {
    TEMP_DANGER = value;
    Serial.printf("Blynk DANGER threshold: %d C\n", value);
  } else {
    Blynk.virtualWrite(V5, (int)TEMP_DANGER);
  }
}

BLYNK_WRITE(V7) { //nhận lệnh đóng/mở và phản hồi trạng thái cửa
  int value = param.asInt();
  if (value == 0 || value == 1) {
    remoteDoorOpenRequested = (value == 1);
    remoteDoorCommandPending = true;
  } else {
    doorStateDirty = true;
  }
}

BLYNK_CONNECTED() { //hàm này chạy khi kết nối blynk thành công là nó gọi hàm V4, V5
  Serial.println("Blynk Cloud connected!");

  // Restore the last thresholds set in the dashboard.
  // Do not sync V7: stale remote commands must not override the real door.
  Blynk.syncVirtual(V4, V5);
  doorStateDirty = true;
  lastCloudSend = 0;
}

// Only attempt a cloud connection briefly; the fire alarm must work offline.
void maintainBlynkConnection() {// duy trì kết nối
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiAttempt >= WIFI_RETRY_INTERVAL) {
      lastWiFiAttempt = millis();
      WiFi.reconnect();
    }
    return;
  }

  if (Blynk.connected()) {
    Blynk.run();
  } else if (firstCloudAttempt ||
             millis() - lastCloudAttempt >= CLOUD_RETRY_INTERVAL) {
    firstCloudAttempt = false;
    lastCloudAttempt = millis();
    Blynk.connect(250);
  }
}

void sendBlynkData(float temperature,//gửi dữ liệu lên blynk
                   bool flameDetected,
                   bool smokeDetected,
                   const String &status) {
  if (!Blynk.connected()) return;

  // Keep both dashboard door widgets in sync with the physical servo state.
  if (doorStateDirty) {
    Blynk.virtualWrite(V6, doorOpen ? 1 : 0);
    Blynk.virtualWrite(V7, doorOpen ? 1 : 0);
    doorStateDirty = false;
  }

  if (millis() - lastCloudSend >= CLOUD_SEND_INTERVAL) {
    lastCloudSend = millis();
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, flameDetected ? 1 : 0);
    Blynk.virtualWrite(V2, smokeDetected ? 1 : 0);
    Blynk.virtualWrite(V3, status);
    
    static String lastStatusColor = "";
    if (status != lastStatusColor) {
      if (status == "SAFE") {
        Blynk.setProperty(V3, "color", "#00C853");
      }
      else if (status == "WARNING") {
        Blynk.setProperty(V3, "color", "#FFC107");
      }
      else if (status == "DANGER") {
        Blynk.setProperty(V3, "color", "#FF1744");
      }
      lastStatusColor = status;
    }

    Blynk.virtualWrite(V6, doorOpen ? 1 : 0);
    }
  }

// ============ SETUP ============

void setup() {
  Serial.begin(115200);

  // Sensor inputs
  pinMode(FLAME_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(LM35_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Outputs
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  // Initial state
  digitalWrite(BUZZER_PIN, LOW);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  // ADC setup
  analogReadResolution(12);
  analogSetPinAttenuation(LM35_PIN, ADC_0db);

  // I2C initialization
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  if (checkI2C(0x27)) {
    lcdAddress = 0x27;
    lcdAvailable = true;
  }
  else if (checkI2C(0x3F)) {
    lcdAddress = 0x3F;
    lcdAvailable = true;
  }

  if (lcdAvailable) {
    Serial.print("LCD found at 0x");
    Serial.println(lcdAddress, HEX);

    lcdInitialize();

    lcdPrintLine(0, "FIRE DETECTION");
    lcdPrintLine(1, "SYSTEM START");

    delay(2000);
  }
  else {
    Serial.println("LCD NOT FOUND!");
  }

  // Servo configuration
  doorServo.setPeriodHertz(50);
  doorServo.attach(SERVO_PIN, 500, 2400);

  // Initially closed
  doorServo.write(DOOR_CLOSE_ANGLE);
  doorOpen = false;

  lastButtonReading = digitalRead(BUTTON_PIN);
  stableButtonState = lastButtonReading;

  // Wi-Fi/Blynk: do not block setup waiting for Internet.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, pass);
  lastWiFiAttempt = millis();
  Blynk.config(BLYNK_AUTH_TOKEN);
  Serial.println("Wi-Fi connection started...");
}

// ============ MAIN LOOP ============

void loop() {

  // 1. Read sensors
  bool flameDetected =
      digitalRead(FLAME_PIN) == FLAME_ACTIVE;

  bool smokeDetected =
      digitalRead(MQ2_PIN) == MQ2_ACTIVE;

  float temperature = readTemperature();

  // 2. Determine danger
  bool danger = flameDetected ||
                smokeDetected ||
                temperature >= TEMP_DANGER;
  
  updateDoor(danger);


  // 3. Determine status
  String status;

  if (danger) {
    status = "DANGER";
  }
  else if (temperature >= TEMP_WARNING) {
    status = "WARNING";
  }
  else {
    status = "SAFE";
  }

  // 4. Control LED and buzzer
  controlOutputs(danger, temperature);

  // 5. LCD display
  String line1 = "Temp: " +
                 String(temperature, 1) + " C";

  String line2 = "F:" +
                 String(flameDetected ? 1 : 0) +
                 " S:" +
                 String(smokeDetected ? 1 : 0) +
                 " " + status;

  lcdPrintLine(0, line1);
  lcdPrintLine(1, line2);

  // 6. Serial Monitor
  Serial.println("====== FIRE SYSTEM ======");

  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");

  Serial.print("Flame: ");
  Serial.println(flameDetected ? "YES" : "NO");

  Serial.print("Smoke: ");
  Serial.println(smokeDetected ? "YES" : "NO");

  Serial.print("Status: ");
  Serial.println(status);

  Serial.print("Buzzer D15: ");

  if (danger) {
    Serial.println("CONTINUOUS");
  }
  else if (temperature >= TEMP_WARNING) {
    Serial.println("INTERMITTENT");
  }
  else {
    Serial.println("OFF");
  }

  Serial.println("=========================");

  // Local warning outputs are updated BEFORE servicing Wi-Fi/cloud.
  maintainBlynkConnection();

  // Re-apply any threshold changes received in Blynk.run() this cycle.
  bool dangerNow = flameDetected || smokeDetected ||
                   temperature >= TEMP_DANGER;
  String statusNow = dangerNow ? "DANGER" :
                     (temperature >= TEMP_WARNING ? "WARNING" : "SAFE");
  if (dangerNow != danger || statusNow != status) {
    danger = dangerNow;
    status = statusNow;
    if (danger) setDoor(true); // Protect door when threshold drops.
    controlOutputs(danger, temperature);
    lcdPrintLine(1, "F:" + String(flameDetected ? 1 : 0) +
                    " S:" + String(smokeDetected ? 1 : 0) + " " + status);
  }

  // Apply remote Door Control using the newest danger state.
  if (remoteDoorCommandPending) {
    remoteDoorCommandPending = false;
    if (danger) {
      setDoor(true); // Never close the door while there is DANGER.
    } else {
      setDoor(remoteDoorOpenRequested);
    }
    doorStateDirty = true; // Restore V7 if a CLOSE request was rejected.
  }

  sendBlynkData(temperature, flameDetected, smokeDetected, status);
  delay(50);
}

// ============== SERVO SETUP ===============

void setDoor(bool open) {

  if (doorOpen == open) return;

  doorOpen = open;
  doorStateDirty = true;

  if (open) {
    doorServo.write(DOOR_OPEN_ANGLE);
    Serial.println("DOOR OPEN");
  } else {
    doorServo.write(DOOR_CLOSE_ANGLE);
    Serial.println("DOOR CLOSED");
  }
}

void updateDoor(bool danger) {

  // DANGER always forces the door open
  if (danger) {
    setDoor(true);
  }

  // Read button
  bool reading = digitalRead(BUTTON_PIN);

  // Detect button signal change
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  // Debounce button
  if (millis() - lastDebounceTime >= DEBOUNCE_DELAY) {

    if (reading != stableButtonState) {
      stableButtonState = reading;

      // A new press (LOW)
      if (stableButtonState == LOW) {

        // Manual control only outside DANGER
        if (!danger) {
          setDoor(!doorOpen);
        }
      }
    }
  }

  lastButtonReading = reading;
}

s
