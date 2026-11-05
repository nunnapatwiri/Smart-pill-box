ผู้พัฒนาโครงงาน:
	1. นางสาวธัชญา เธียรประดับโชค 6810503617
	2. นางสาวนันท์นภัส วิริยะวัฒนา 6810503650
	3. นางสาวนิชาภา มาทิพย์ 6810503668
วิชา: การพัฒนาฮาร์ดแวร์คอมพิวเตอร์เบื้องต้น (Introduction to Computer Hardware Development)
ภาควิศวกรรมคอมพิวเตอร์ คณะวิศวกรรมศาสตร์ มหาวิทยาลัยเกษตรศาสตร์

ไฟล์ของโครงงาน
HR Watch โฟลเดอร์ที่มีโฟลเดอร์ bpm ในนั้นมี bpm.ino ไฟล์โค้ดของนาฬิกาวัดอัตราการเต้นของหัวใจ
Pill Box โฟลเดอร์ที่มีโฟลเดอร์ Application และ Pillbox_ฺBoard
	โฟลเดอร์ Application เกี่ยวกับซอร์สโค้ดแอปพลิเคชัน Flutter
        โฟลเดอร์  Pillbox_board มี Pillbox_board.ino ไฟล์โค้ดของกล่องยา
Schematic โฟลเดอร์ที่เก็บภาพผังวงจรบอร์ดที่ 1 และบอร์ดที่ 2 ของฮาร์ดแวร์
heartrate_flows.json ไฟล์ของ Node-RED ใช้แสดง DashBoard
README.txt ไฟล์อธิบายรายละเอียดของโครงงาน โครงสร้างไฟล์ ผู้จัดทำ รายละเอียดไฟล์ และรายการไลบรารีและอุปกรณ์ฮาร์ดแวร์ที่ใช้
license.txt ไฟล์อธิบายเงื่อนไขการอนุญาตให้ผู้อื่นนำโครงงานนี้ไปใช้

รายการไลบรารีและอุปกรณ์ฮาร์ดแวร์
ไลบรารี
	•	Wire.h (สำหรับการสื่อสารแบบ I2C)
	•	WiFi.h (สำหรับเชื่อมต่อ WiFi)
	•	PubSubClient (สำหรับสื่อสารผ่าน MQTT)
	•	Adafruit SSD1306 (สำหรับควบคุมจอ OLED)
	•	Adafruit GFX (สำหรับแสดงผลกราฟิก)
	•	MAX30102 Library (สำหรับอ่านค่าจากเซนเซอร์วัดชีพจร)
	•	ArduinoJson.h (สำหรับจัดการข้อมูล JSON)
	•	HTTPClient.h (สำหรับส่ง HTTP request )
ซอฟต์แวร์
	•	Arduino IDE (ใช้พัฒนาโปรแกรมบน ESP32)
	•	Node-RED (ใช้ประมวลผลข้อมูลและควบคุมระบบ)
	•	Node-RED Dashboard (ใช้แสดงผลข้อมูล)
	•	Flutter (ใช้พัฒนาแอปพลิเคชันบนมือถือ)
	•	MQTT Protocol (ใช้สำหรับการสื่อสารข้อมูลในระบบ IoT)
	•	LINE Messaging API / LINE Notify (ใช้สำหรับส่งการแจ้งเตือนให้ผู้ใช้งาน)
อุปกรณ์ฮาร์ดแวร์
Board I:
	•	ESP32-S3 Microcontroller
	•	LDR Light Sensor
	•	Vibration Motor Module
	•	Buzzer Module
	•	LED
	•	OLED I2C Display
Board II:
	•	ESP32-S3 Microcontroller
	•	MAX30102 Heart Rate Sensor
	•	OLED I2C Display

