#ifndef CONFIG_H
#define CONFIG_H

// ===== WIFI =====
const char* ssid     = "Nortor";
const char* password = "0996255123";

// ===== MQTT =====
const char* mqtt_broker   = "iot.cpe.ku.ac.th";
const int   mqtt_port     = 1883;
const char* mqtt_user     = "b6810503650";
const char* mqtt_pass     = "nunnapat.wi@ku.th";
const char* topic_sub_pill = "b6810503650/pillbox/config";
const char* topic_pub_status = "b6810503650/pillbox/status";

// ===== PIN =====
#define I2C_SDA 48
#define I2C_SCL 47
#define PIN_BUZZER 18
#define PIN_VIBRATE 8
#define PIN_LDR 4
#define PIN_LEDONE 42
#define PIN_LEDTWO 41
#define PIN_LEDTHREE 40

// ===== OLED =====
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C

#endif
