#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <DHT.h>
#include <TFT_22_ILI9225.h>
#include <ESP32Servo.h>

// WiFi
#define WIFI_SSID "tuan"
#define WIFI_PASSWORD "1234567890"

// Firebase
#define DATABASE_URL "https://stupid-city-default-rtdb.firebaseio.com"
#define DATABASE_SECRET "yPygn8AgTeBWG2gM7XPubgo6z3FCNBhPYyrPER5N"

// Firebase client
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Chân cảm biến
#define DHTPIN 21
#define DHTTYPE DHT11
#define MQ2_PIN 34

// TFT ILI9225 pins
#define TFT_RST 4
#define TFT_RS 2
#define TFT_CS 15
#define TFT_SDI 23
#define TFT_CLK 18

// Thiết bị điều khiển
#define FAN_PIN     13
#define BUZZER_PIN  12
#define SERVO_LEFT  14
#define SERVO_RIGHT 27

// Nút bấm
#define ONOFF_BUTTON 32
#define UP_BUTTON    33
#define DOWN_BUTTON  25
#define MODE_BUTTON  26

// Biến trạng thái
bool autoMode = true;
bool settingMode = false; // false: không cài đặt, true: đang cài đặt ngưỡng
int settingType = 0; // 0: none, 1: gas, 2: temp
int gasThreshold = 400;
int tempThreshold = 40;
bool fanState = false;
bool buzzerState = false;
bool doorState = false;

unsigned long lastModePress = 0;
unsigned long lastOnOffPress = 0;
unsigned long lastUpDownPress = 0;
const unsigned long DEBOUNCE_DELAY = 300;

DHT dht(DHTPIN, DHTTYPE);
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_SDI, TFT_CLK);
Servo servoLeft, servoRight;

void setup() {
  Serial.begin(115200);
  dht.begin();
  tft.begin();
  tft.setOrientation(1);
  tft.setFont(Terminal6x8); // phông chữ nhỏ
  tft.clear();

  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n Đã kết nối WiFi");

  // Cấu hình Firebase
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Cấu hình chân thiết bị
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(ONOFF_BUTTON, INPUT_PULLUP);
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  pinMode(MODE_BUTTON, INPUT_PULLUP);

  servoLeft.attach(SERVO_LEFT);
  servoRight.attach(SERVO_RIGHT);
  servoLeft.write(90);
  servoRight.write(0);

  // Đọc giá trị ban đầu từ Firebase
  if (Firebase.RTDB.getBool(&fbdo, "/setting/auto_mode")) {
    autoMode = fbdo.boolData();
    Serial.println(autoMode ? " Khởi tạo: Auto Mode" : " Khởi tạo: Manual Mode");
  }
  if (Firebase.RTDB.getInt(&fbdo, "/setting/gas_threshold")) {
    gasThreshold = fbdo.intData();
  }
  if (Firebase.RTDB.getInt(&fbdo, "/setting/temp_threshold")) {
    tempThreshold = fbdo.intData();
  }
}

void handleModeButton() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(MODE_BUTTON);
  unsigned long now = millis();

  if (lastState == HIGH && currentState == LOW && now - lastModePress > DEBOUNCE_DELAY) {
    lastModePress = now;
    if (settingMode) {
      settingType = (settingType == 1) ? 2 : 1; // Chuyển giữa gas và temp
      Serial.println(settingType == 1 ? " Chế độ cài đặt: Gas Threshold" : " Chế độ cài đặt: Temp Threshold");
    } else {
      autoMode = !autoMode;
      Firebase.RTDB.setBool(&fbdo, "/setting/auto_mode", autoMode);
      Serial.println(autoMode ? " Chuyển sang Auto Mode" : " Chuyển sang Manual Mode");
    }
  }
  lastState = currentState;
}

void handleOnOffButton() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(ONOFF_BUTTON);
  unsigned long now = millis();

  if (lastState == HIGH && currentState == LOW && now - lastOnOffPress > DEBOUNCE_DELAY) {
    lastOnOffPress = now;
    if (autoMode) {
      settingMode = !settingMode;
      settingType = settingMode ? 1 : 0; // Bắt đầu với gas nếu vào chế độ cài đặt
      Serial.println(settingMode ? " Vào chế độ cài đặt ngưỡng" : " Thoát chế độ cài đặt");
    } else {
      doorState = !doorState;
      Firebase.RTDB.setBool(&fbdo, "/control/door", doorState);
      Serial.println(doorState ? " Mở cửa" : " Đóng cửa");
    }
  }
  lastState = currentState;
}

void handleUpDownButtons() {
  unsigned long now = millis();
  if (now - lastUpDownPress < DEBOUNCE_DELAY) return;

  if (digitalRead(UP_BUTTON) == LOW) {
    lastUpDownPress = now;
    if (settingMode && settingType == 1) {
      gasThreshold += 10;
      Firebase.RTDB.setInt(&fbdo, "/setting/gas_threshold", gasThreshold);
      Serial.println(" Tăng ngưỡng gas: " + String(gasThreshold) + " ppm");
    } else if (settingMode && settingType == 2) {
      tempThreshold += 1;
      Firebase.RTDB.setInt(&fbdo, "/setting/temp_threshold", tempThreshold);
      Serial.println(" Tăng ngưỡng nhiệt độ: " + String(tempThreshold) + " °C");
    } else if (!autoMode) {
      fanState = !fanState;
      Firebase.RTDB.setBool(&fbdo, "/control/fan", fanState);
      Serial.println(fanState ? " Bật quạt" : " Tắt quạt");
    }
  }

  if (digitalRead(DOWN_BUTTON) == LOW) {
    lastUpDownPress = now;
    if (settingMode && settingType == 1) {
      gasThreshold = max(100, gasThreshold - 10); 
      Firebase.RTDB.setInt(&fbdo, "/setting/gas_threshold", gasThreshold);
      Serial.println(" Giảm ngưỡng gas: " + String(gasThreshold) + " ppm");
    } else if (settingMode && settingType == 2) {
      tempThreshold = max(10, tempThreshold - 1); 
      Firebase.RTDB.setInt(&fbdo, "/setting/temp_threshold", tempThreshold);
      Serial.println(" Giảm ngưỡng nhiệt độ: " + String(tempThreshold) + " °C");
    } else if (!autoMode) {
      buzzerState = !buzzerState;
      Firebase.RTDB.setBool(&fbdo, "/control/buzzer", buzzerState);
      Serial.println(buzzerState ? " Bật còi" : " Tắt còi");
    }
  }
}

void updateDevices() {
  digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
  servoLeft.write(doorState ? 0 : 90);
  servoRight.write(doorState ? 90 : 0);
}

void loop() {
  handleModeButton();
  handleOnOffButton();
  handleUpDownButtons();

  // Đọc dữ liệu cảm biến
  float temp = dht.readTemperature();
  float humi = dht.readHumidity();
  int gasValue = analogRead(MQ2_PIN);

  if (isnan(temp) || isnan(humi)) {
    Serial.println(" Không đọc được DHT11!");
    tft.clear();
    tft.drawText(0, 0, "Error: DHT11 failed!", COLOR_RED);
    delay(2000);
    return;
  }

  // Gửi dữ liệu cảm biến lên Firebase
  Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", temp);
  Firebase.RTDB.setFloat(&fbdo, "/sensor/humidity", humi);
  Firebase.RTDB.setInt(&fbdo, "/sensor/gas", gasValue);

  // Đọc ngưỡng và chế độ từ Firebase
  bool prevAutoMode = autoMode;
  if (Firebase.RTDB.getBool(&fbdo, "/setting/auto_mode")) {
    autoMode = fbdo.boolData();
    if (autoMode != prevAutoMode) {
      Serial.println(autoMode ? " Firebase: Chuyển sang Auto Mode" : " Firebase: Chuyển sang Manual Mode");
    }
  }
  if (Firebase.RTDB.getInt(&fbdo, "/setting/gas_threshold")) {
    gasThreshold = fbdo.intData();
  }
  if (Firebase.RTDB.getInt(&fbdo, "/setting/temp_threshold")) {
    tempThreshold = fbdo.intData();
  }

  // Chế độ Auto: Điều khiển tự động
  if (autoMode && !settingMode) {
    fanState = (temp > tempThreshold);
    buzzerState = (gasValue > gasThreshold);
    Firebase.RTDB.setBool(&fbdo, "/control/fan", fanState);
    Firebase.RTDB.setBool(&fbdo, "/control/buzzer", buzzerState);
    Firebase.RTDB.setBool(&fbdo, "/control/door", doorState);
  } else {
    // Chế độ Manual hoặc Setting: Đọc trạng thái từ Firebase
    if (Firebase.RTDB.getBool(&fbdo, "/control/fan")) {
      fanState = fbdo.boolData();
    }
    if (Firebase.RTDB.getBool(&fbdo, "/control/buzzer")) {
      buzzerState = fbdo.boolData();
    }
    if (Firebase.RTDB.getBool(&fbdo, "/control/door")) {
      doorState = fbdo.boolData();
    }
  }
 
  updateDevices();

  // Hiển thị thông tin lên Serial
  Serial.printf(" Nhiệt độ: %.1f °C |  Độ ẩm: %.1f %% |  Gas: %d ppm\n", temp, humi, gasValue);
  Serial.printf("Ngưỡng: Gas %d ppm | Nhiệt độ %d °C\n", gasThreshold, tempThreshold);
  Serial.printf("Quạt: %s | Còi: %s | Cửa: %s\n", 
                fanState ? "BẬT" : "TẮT", 
                buzzerState ? "BẬT" : "TẮT", 
                doorState ? "MỞ" : "ĐÓNG");
  Serial.printf("Chế độ: %s | Cài đặt: %s\n\n", 
                autoMode ? "Auto" : "Manual", 
                settingMode ? (settingType == 1 ? "Gas" : "Temp") : "Không");

  // Hiển thị lên TFT
  tft.clear();
  tft.drawText(5, 0,  "Nhiet: " + String(temp, 1) + " C", COLOR_WHITE);
  tft.drawText(5, 16, "Do am: " + String(humi, 1) + " %", COLOR_WHITE);
  tft.drawText(5, 32, "Gas: " + String(gasValue) + " ppm", COLOR_WHITE);
  tft.drawText(5, 48, "Ng. nhiet: " + String(tempThreshold) + " C", COLOR_YELLOW);
  tft.drawText(5, 64, "Ng. gas: " + String(gasThreshold) + " ppm", COLOR_YELLOW);
  tft.drawText(5, 80, "Quat: " + String(fanState ? "Bat" : "Tat"), COLOR_GREEN);
  tft.drawText(5, 96, "Coi: " + String(buzzerState ? "Bat" : "Tat"), COLOR_RED);
  tft.drawText(5, 112, "Cua: " + String(doorState ? "Mo" : "Dong"), COLOR_CYAN);
  tft.drawText(5, 128, "Mode: " + String(autoMode ? "Auto" : "Manual"), COLOR_ORANGE);
  tft.drawText(5, 144, settingMode ? (settingType == 1 ? "Set: Gas" : "Set: Temp") : "Set: None", COLOR_WHITE);
  
  delay(1000);
}