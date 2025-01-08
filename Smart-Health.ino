#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MAX30100.h>
#include <RTClib.h>

#define LM35_PIN A0
#define BUZZER_PIN D5
#define BUTTON_NEXT D6
#define BUTTON_PREV D7
#define BUTTON_SELECT D8

// Create instances
LiquidCrystal_I2C lcd(0x27, 20, 4);
ESP8266WebServer server(80);
Adafruit_MAX30100 pulseOximeter;
RTC_DS3231 rtc;

// WiFi credentials
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// Navigation states
enum State {
  MAIN,
  HEART_RATE,
  SPO2,
  TEMPERATURE,
  DATA_DISPLAY,
  SERIAL_NUMBER
};
State currentState = MAIN;

// User data structure
struct UserData {
  char name[30];
  int age;
  float height;
  char healthProblems[100];
  float weight;
  char emergencyNumber[15];
  char doctorNumber[15];
  char serialNumber[20];
};

UserData userData;

void saveUserData() {
  EEPROM.put(0, userData);
  EEPROM.commit();
}

void loadUserData() {
  EEPROM.get(0, userData);
}

void handleRoot() {
  String html = "<html><head><style>";
  html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; }";
  html += "form { max-width: 500px; margin: auto; padding: 1em; background: #ffffff; border-radius: 5px; }";
  html += "input[type='text'], input[type='number'] { width: 100%; padding: 8px; margin: 8px 0; box-sizing: border-box; }";
  html += "input[type='submit'] { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; cursor: pointer; }";
  html += "h1 { text-align: center; background: #4CAF50; color: white; padding: 10px; margin: 0; font-size: 24px; }";
  html += "footer { text-align: center; background: #4CAF50; color: white; padding: 10px; margin: 0; font-size: 14px; }";
  html += "</style></head><body>";
  html += "<h1>Smart Health</h1>";
  html += "<form action='/save' method='POST'>";
  html += "Name: <input type='text' name='name'><br>";
  html += "Age: <input type='number' name='age'><br>";
  html += "Height (cm): <input type='number' name='height'><br>";
  html += "Health Problems: <input type='text' name='healthProblems'><br>";
  html += "Weight (kg): <input type='number' name='weight'><br>";
  html += "Emergency Number: <input type='text' name='emergencyNumber'><br>";
  html += "Doctor's Number: <input type='text' name='doctorNumber'><br>";
  html += "Serial Number (Password): <input type='text' name='serialNumber'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form>";
  html += "<footer>فريق الإنقاذ مشروع قياس العمليات الحيوية</footer>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("name")) strcpy(userData.name, server.arg("name").c_str());
  if (server.hasArg("age")) userData.age = server.arg("age").toInt();
  if (server.hasArg("height")) userData.height = server.arg("height").toFloat();
  if (server.hasArg("healthProblems")) strcpy(userData.healthProblems, server.arg("healthProblems").c_str());
  if (server.hasArg("weight")) userData.weight = server.arg("weight").toFloat();
  if (server.hasArg("emergencyNumber")) strcpy(userData.emergencyNumber, server.arg("emergencyNumber").c_str());
  if (server.hasArg("doctorNumber")) strcpy(userData.doctorNumber, server.arg("doctorNumber").c_str());
  if (server.hasArg("serialNumber")) strcpy(userData.serialNumber, server.arg("serialNumber").c_str());

  saveUserData();
  server.send(200, "text/html", "<html><body><h1>Data Saved!</h1><a href='/'>Go Back</a><footer>فريق الإنقاذ مشروع قياس العمليات الحيوية</footer></body></html>");
}

void playBuzzer() {
  int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
  int noteDurations[] = {4, 4, 4, 4, 4, 4, 4, 4};

  for (int thisNote = 0; thisNote < 8; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BUZZER_PIN, melody[thisNote], noteDuration);
    delay(noteDuration * 1.30);
  }
  noTone(BUZZER_PIN);
}

void displayMainScreen() {
  playBuzzer();
  DateTime now = rtc.now();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(0)); // Clock Icon
  lcd.setCursor(1, 0);
  lcd.print("Date: ");
  lcd.print(now.day());
  lcd.print("/");
  lcd.print(now.month());
  lcd.print("/");
  lcd.print(now.year());
  lcd.setCursor(0, 1);
  lcd.print("Time: ");
  lcd.print(now.hour());
  lcd.print(":");
  lcd.print(now.minute());
}

void displayHeartRate() {
  playBuzzer();
  float pulseRate;
  pulseOximeter.update();
  pulseRate = pulseOximeter.getHeartRate();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(1)); // Heart Icon
  lcd.setCursor(1, 0);
  lcd.print("Heart Rate:");
  lcd.setCursor(0, 1);
  lcd.print(pulseRate);
  lcd.print(" bpm");
}

void displaySpO2() {
  playBuzzer();
  float spO2;
  pulseOximeter.update();
  spO2 = pulseOximeter.getSpO2();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(2)); // O2 Icon
  lcd.setCursor(1, 0);
  lcd.print("SpO2 Level:");
  lcd.setCursor(0, 1);
  lcd.print(spO2);
  lcd.print("%");
}

void displayTemperature() {
  playBuzzer();
  float temperature = (analogRead(LM35_PIN) * 3.3 / 1023.0) * 100.0; // LM35 conversion
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(5)); // Temperature Icon
  lcd.setCursor(1, 0);
  lcd.print("Temperature:");
  lcd.setCursor(0, 1);
  lcd.print(temperature);
  lcd.print(" C");
}

void displayUserData() {
  playBuzzer();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(3)); // User Icon
  lcd.setCursor(1, 0);
  lcd.print("Name: ");
  lcd.print(userData.name);
  lcd.setCursor(0, 1);
  lcd.print("Age: ");
  lcd.print(userData.age);
  lcd.setCursor(0, 2);
  lcd.print("Weight: ");
  lcd.print(userData.weight);
}

void displaySerialNumber() {
  playBuzzer();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(4)); // Serial Number Icon
  lcd.setCursor(1, 0);
  lcd.print("Serial No:");
  lcd.setCursor(0, 1);
  lcd.print(userData.serialNumber);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  loadUserData();

  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_PREV, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Custom Icons
  byte clockIcon[8] = {0x00, 0x0E, 0x11, 0x15, 0x15, 0x11, 0x0E, 0x00};
  byte heartIcon[8] = {0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00};
  byte o2Icon[8] = {0x00, 0x0E, 0x11, 0x11, 0x1F, 0x04, 0x0E, 0x00};
  byte userIcon[8] = {0x00, 0x0E, 0x0E, 0x00, 0x0E, 0x15, 0x0E, 0x00};
  byte snIcon[8] = {0x00, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x1F, 0x00};
  byte tempIcon[8] = {0x00, 0x04, 0x0E, 0x0E, 0x0E, 0x1F, 0x1F, 0x0E};
  lcd.createChar(0, clockIcon);
  lcd.createChar(1, heartIcon);
  lcd.createChar(2, o2Icon);
  lcd.createChar(3, userIcon);
  lcd.createChar(4, snIcon);
  lcd.createChar(5, tempIcon);

  // Initialize MAX30100
  if (!pulseOximeter.begin()) {
    Serial.println("MAX30100 initialization failed");
    while (1);
  }

  pulseOximeter.setMode(MAX30100_MODE_SPO2_HR);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/save", handleSave);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  if (digitalRead(BUTTON_NEXT) == LOW) {
    currentState = (State)((currentState + 1) % 6);
    playBuzzer();
    delay(200);
  }

  if (digitalRead(BUTTON_PREV) == LOW) {
    currentState = (State)((currentState - 1 + 6) % 6);
    playBuzzer();
    delay(200);
  }

  if (digitalRead(BUTTON_SELECT) == LOW) {
    playBuzzer();
    switch (currentState) {
      case HEART_RATE:
        displayHeartRate();
        break;
      case SPO2:
        displaySpO2();
        break;
      case TEMPERATURE:
        displayTemperature();
        break;
      case DATA_DISPLAY:
        displayUserData();
        break;
      case SERIAL_NUMBER:
        displaySerialNumber();
        break;
      default:
        break;
    }
    delay(2000);
  }

  if (currentState == MAIN) {
    displayMainScreen();
  }
}
