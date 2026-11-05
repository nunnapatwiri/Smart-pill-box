#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"

// ================= WIFI =================
const char* ssid = "ขอตอ";
const char* password = "oooooooo";

// ================= MQTT =================
const char* mqtt_server = "iot.cpe.ku.ac.th";
const int mqtt_port = 1883;
const char* mqtt_user = "b6810503668";
const char* mqtt_pass = "nichapa.ma@ku.th";
const char* topic_bpm = "b6810503668/bpm";

// ================= LINE =================
String CHANNEL_ACCESS_TOKEN =
"ZOPRgl0AXV8R1yuHgM58uBBftYP8jouHdhf+ScSeSs1CTrlvu717JGdy9oJv1lq89ubsLcsCA/gl1E4Ig+ltJ+vve1euu109IIfmg4/IhGfMKMQqqQPQnrpHPkBFmdExQaxyaYy8Wbsvj4rP+WCM1QdB04t89/1O/w1cDnyilFU=";

String USER_ID =
"U943c63b7a38984f3eb11a5d06fee6a1c";

String USER_ID_2 =
"Uee493e740cd88477d79771cc6bf6a1aa";

String USER_ID_3 =
"Ud1155effa2e8e547501fb06e2031fdda";

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 47
#define OLED_SCL 48

TwoWire I2C_OLED = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, -1);

// ================= MAX30102 =================
TwoWire I2C_MAX = TwoWire(1);
MAX30105 particleSensor;

// ================= MQTT =================
WiFiClient espClient;
PubSubClient client(espClient);

// ================= BPM =================
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
int beatAvg = 0;

// ================= SYSTEM CONTROL =================
bool systemReady = false;
bool alertSent = false;


// ================= LINE FUNCTION =================
void sendLine(String msg){

  if(WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  http.begin("https://api.line.me/v2/bot/message/push");
  http.addHeader("Content-Type","application/json");
  http.addHeader("Authorization","Bearer " + CHANNEL_ACCESS_TOKEN);

  String payload =
  "{\"to\":\""+USER_ID+
  "\",\"messages\":[{\"type\":\"text\",\"text\":\""+msg+"\"}]}";

  int code = http.POST(payload);

  Serial.print("LINE code: ");
  Serial.println(code);

  payload =
  "{\"to\":\""+USER_ID_2+
  "\",\"messages\":[{\"type\":\"text\",\"text\":\""+msg+"\"}]}";

  code = http.POST(payload);

  Serial.print("LINE code 2: ");
  Serial.println(code);

  payload =
  "{\"to\":\""+USER_ID_3+
  "\",\"messages\":[{\"type\":\"text\",\"text\":\""+msg+"\"}]}";

  code = http.POST(payload);

  Serial.print("LINE code 3: ");
  Serial.println(code);

  http.end();
}


// ================= ALERT FUNCTION =================
void checkHeartRateAlert(int bpm){

  if(!systemReady){

    if(bpm >= 60 && bpm <= 100){
      systemReady = true;
    }

    return;
  }

  if(bpm > 100){

    if(!alertSent){

      sendLine("ชีพจรสูงผิดปกติ: " + String(bpm) + " BPM");

      alertSent = true;
    }
  }

  else if(bpm < 60){

    if(!alertSent){

      sendLine("ชีพจรต่ำผิดปกติ: " + String(bpm) + " BPM");

      alertSent = true;
    }
  }

  else{
    alertSent = false;
  }
}


void setup(){

  Serial.begin(115200);
  delay(2000);

  I2C_OLED.begin(OLED_SDA, OLED_SCL);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("OLED not found");
    while(1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Starting...");
  display.display();

  delay(2000);

  display.clearDisplay();
  display.display();

  I2C_MAX.begin(8,9);

  if(!particleSensor.begin(I2C_MAX, I2C_SPEED_FAST)){
    Serial.println("MAX30102 not found");
    while(1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x3F);
  particleSensor.setPulseAmplitudeGreen(0);

  Serial.println("Sensor ready");

  WiFi.begin(ssid,password);

  Serial.print("Connecting WiFi");

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");

  client.setServer(mqtt_server, mqtt_port);

  while(!client.connected()){
    Serial.print("Connecting MQTT...");
    if(client.connect("esp32s3", mqtt_user, mqtt_pass)){
      Serial.println("connected");
    }else{
      Serial.println("failed");
      delay(2000);
    }
  }

  Serial.println("MQTT connected");
}


void loop(){

  if(!client.connected()){
    client.connect("esp32s3", mqtt_user, mqtt_pass);
  }

  client.loop();

  long irValue = particleSensor.getIR();

  if(irValue < 50000){

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("Ready to");
    display.println("Measure HR");
    display.display();

    delay(200);
    return;
  }

  if(checkForBeat(irValue)){

    long delta = millis() - lastBeat;
    lastBeat = millis();

    float bpm = 60 / (delta / 1000.0);

    if(bpm < 180 && bpm > 40){

      rates[rateSpot++] = (byte)bpm;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;

      for(byte x=0;x<RATE_SIZE;x++)
        beatAvg += rates[x];

      beatAvg /= RATE_SIZE;

      Serial.print("BPM: ");
      Serial.println(beatAvg);

      checkHeartRateAlert(beatAvg);

      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0,0);
      display.print("<3 ");
      display.print(beatAvg);
      display.println(" BPM");
      display.display();

      char bpmStr[8];
      itoa(beatAvg, bpmStr, 10);
      client.publish(topic_bpm, bpmStr);
    }
  }

  delay(20);
}