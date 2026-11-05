#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>  // ตัวสำคัญสำหรับ LINE
#include "config.h"

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// --- LINE Configuration ---
String CHANNEL_ACCESS_TOKEN = "ZOPRgl0AXV8R1yuHgM58uBBftYP8jouHdhf+ScSeSs1CTrlvu717JGdy9oJv1lq89ubsLcsCA/gl1E4Ig+ltJ+vve1euu109IIfmg4/IhGfMKMQqqQPQnrpHPkBFmdExQaxyaYy8Wbsvj4rP+WCM1QdB04t89/1O/w1cDnyilFU=";
String USER_ID = "U943c63b7a38984f3eb11a5d06fee6a1c";
String USER_ID_2 = "Uee493e740cd88477d79771cc6bf6a1aa";
String USER_ID_3 = "Ud1155effa2e8e547501fb06e2031fdda";

// --- ตัวแปรควบคุมสถานะ (Logic เดิมของเนส) ---
bool alarmActive = false;
bool waitingToClear = false; 
unsigned long alarmStartTime = 0;
unsigned long takenTime = 0; 
const long alarmDuration = 30000;
const long clearDelay = 60000;    
unsigned long lastToggleTime = 0;
const long toggleInterval = 1000; 
bool isBuzzing = false;
String targetSlot = ""; 
String currentPillName = ""; // เก็บชื่อยาเพื่อส่ง LINE

// --- ตัวแปรระบบแสง (Logic เดิมของเนส) ---
int baseLightValue = 0;
int lightConfirmCounter = 0;
const int confirmThreshold = 15; 

// ================= LINE FUNCTION =================
void sendLineMessage(String msg) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin("https://api.line.me/v2/bot/message/push");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + CHANNEL_ACCESS_TOKEN);

  // ส่งหา User 1
  String payload = "{\"to\":\"" + USER_ID + "\",\"messages\":[{\"type\":\"text\",\"text\":\"" + msg + "\"}]}";
  http.POST(payload);

  // ส่งหา User 2 (เพื่อน)
  payload = "{\"to\":\"" + USER_ID_2 + "\",\"messages\":[{\"type\":\"text\",\"text\":\"" + msg + "\"}]}";
  http.POST(payload);
  
  payload = "{\"to\":\"" + USER_ID_3 + "\",\"messages\":[{\"type\":\"text\",\"text\":\"" + msg + "\"}]}";
  http.POST(payload);

  http.end();
}

void updateDisplay(String title, String subtitle) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(title);
  display.setTextSize(2);
  display.setCursor(0, 40);
  display.println(subtitle);
  display.display();
}

void stopAlarmOnly() {
  digitalWrite(PIN_BUZZER, HIGH); // เงียบ
  digitalWrite(PIN_VIBRATE, LOW);  // หยุดสั่น
  isBuzzing = false;
}

void startAlarm() {
  digitalWrite(PIN_BUZZER, LOW);   // ดัง
  digitalWrite(PIN_VIBRATE, HIGH); // สั่น
  
  // ระหว่างร้อง ปิดไฟ LED ทั้งหมดตามโจทย์
  digitalWrite(PIN_LEDONE, LOW);
  digitalWrite(PIN_LEDTWO, LOW);
  digitalWrite(PIN_LEDTHREE, LOW);
  isBuzzing = true;
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("📩 MQTT Received: " + msg);

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, msg) == DeserializationError::Ok) {
    if (doc["buzzer"] == "ON") {
      alarmActive = true;
      waitingToClear = false;
      lightConfirmCounter = 0;
      alarmStartTime = millis(); 
      lastToggleTime = millis(); 
      targetSlot = doc["slot"].as<String>(); 
      currentPillName = doc["pillName"].as<String>(); // เก็บชื่อยา
      
      baseLightValue = analogRead(PIN_LDR); 
      startAlarm();
      updateDisplay(currentPillName, doc["amount"].as<String>() + " tabs");
      client.publish(topic_pub_status, "{\"status\":\"ALARMING\"}");
    }
  }
} 

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_VIBRATE, OUTPUT);
  pinMode(PIN_LEDONE, OUTPUT);
  pinMode(PIN_LEDTWO, OUTPUT);
  pinMode(PIN_LEDTHREE, OUTPUT);
  
  stopAlarmOnly(); // เริ่มต้นให้เงียบ
  digitalWrite(PIN_LEDONE, LOW); digitalWrite(PIN_LEDTWO, LOW); digitalWrite(PIN_LEDTHREE, LOW);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED failed"));
    for(;;);
  }
  updateDisplay("Smart Pill", "Ready...");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ WiFi Connected");

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    // เปลี่ยนชื่อ Client ID ให้ไม่ซ้ำกับของเดิมที่อาจค้างอยู่ใน Server
    String clientId = "ESP32_PillBox_LINE_" + String(random(0, 999));
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      client.subscribe(topic_sub_pill);
    }
  }
  client.loop();

  int currentLDR = analogRead(PIN_LDR);
  
  if (alarmActive) {
    unsigned long currentMillis = millis();
    int lightDiff = baseLightValue - currentLDR;

    // 🚨 1. Logic ตรวจจับการเปิดฝา (Logic เดิมของเนส)
    if (currentMillis - alarmStartTime > 2000) {
      if (lightDiff > 1000) { 
        lightConfirmCounter++;
      } else {
        if (lightConfirmCounter > 0) lightConfirmCounter--;
      }

      if (lightConfirmCounter >= confirmThreshold) { 
        alarmActive = false;
        waitingToClear = true; 
        takenTime = currentMillis; 
        
        stopAlarmOnly();
        
        // ไฟติดค้างตามช่องยา
        if (targetSlot == "slot1") digitalWrite(PIN_LEDONE, HIGH);
        else if (targetSlot == "slot2") digitalWrite(PIN_LEDTWO, HIGH);
        else if (targetSlot == "slot3") digitalWrite(PIN_LEDTHREE, HIGH);
        
        updateDisplay("Status:", "TAKEN!");
        
        // ⭐ แจ้งเตือน LINE ทันทีเมื่อกินยาสำเร็จ
        sendLineMessage("ทานยา " + currentPillName + " เรียบร้อยแล้ว");
        
        client.publish(topic_pub_status, "{\"status\":\"TAKEN\"}");
        Serial.println("✅ Medicine Taken & LINE Notified!");
      }
    }

    // 🚨 2. ระบบแจ้งเตือน (ร้อง/สั่น/Timeout)
    if (alarmActive) {
      if (currentMillis - alarmStartTime >= alarmDuration) {
        alarmActive = false;
        stopAlarmOnly();
        updateDisplay("Status:", "TIMEOUT");
        
        // ⭐ แจ้งเตือน LINE เมื่อลืมทานยา
        sendLineMessage("⚠️ แจ้งเตือน: ลืมทานยา " + currentPillName);
        
        client.publish(topic_pub_status, "{\"status\":\"TIMEOUT\"}");
      } else {
        if (currentMillis - lastToggleTime >= toggleInterval) {
          lastToggleTime = currentMillis;
          if (!isBuzzing) startAlarm();
          else stopAlarmOnly();
        }
      }
    }
  }

  // 🚨 3. ดับไฟและหน้าจอหลังผ่านไป 1 นาที (Logic เดิมของเนส)
  if (waitingToClear) {
    if (millis() - takenTime >= clearDelay) {
      waitingToClear = false;
      digitalWrite(PIN_LEDONE, LOW); digitalWrite(PIN_LEDTWO, LOW); digitalWrite(PIN_LEDTHREE, LOW);
      display.clearDisplay(); display.display();
      Serial.println("🧹 1 Min Passed: System Cleared.");
    }
  }

  // Debug ค่าแสง
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.printf("LDR: %d | Diff: %d | Confirm: %d/%d\n", currentLDR, baseLightValue - currentLDR, lightConfirmCounter, confirmThreshold);
    lastPrint = millis();
  }
  delay(10);
}