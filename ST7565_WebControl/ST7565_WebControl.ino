#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <U8g2lib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// --- 하드웨어 설정 ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 14 // D5 (OLED SDA 고정)
#define OLED_SCL 12 // D6

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// CLK을 D3(GPIO0)으로 변경: D5(GPIO14)는 OLED SDA(Wire)와 충돌함
U8G2_ST7565_ERC12864_F_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/ 0, /* data=*/ 13, /* cs=*/ 16, /* dc=*/ 4, /* reset=*/ 5);

#include "config.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

ESP8266WebServer server(80);

// 전역 상태 변수
String displayText = "WiFi Connecting...";
int lcdBrightness = 800;
int oledBrightness = 127;
int rotationMode = 0; // 0, 1, 2, 3 (90도 단위)

// --- 디스플레이 업데이트 함수 ---
void updateDisplays() {
  // LCD (ST7565) 회전 적용
  switch(rotationMode) {
    case 0: u8g2.setDisplayRotation(U8G2_R0); break;
    case 1: u8g2.setDisplayRotation(U8G2_R1); break;
    case 2: u8g2.setDisplayRotation(U8G2_R2); break;
    case 3: u8g2.setDisplayRotation(U8G2_R3); break;
  }
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf); // 가독성 좋은 폰트로 변경
  u8g2.drawStr(0, 12, "DISPLAY DASHBOARD");
  u8g2.drawLine(0, 15, 128, 15);
  
  u8g2.drawStr(0, 30, "IP:");
  u8g2.drawStr(25, 30, WiFi.localIP().toString().c_str());
  
  u8g2.drawStr(0, 45, "MSG:");
  u8g2.drawStr(0, 58, displayText.c_str());
  u8g2.sendBuffer();

  // OLED (SSD1306) 회전 적용
  oled.setRotation(rotationMode);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("DISPLAY DASHBOARD");
  oled.drawLine(0, 12, 128, 12, SSD1306_WHITE);
  
  oled.setCursor(0, 20);
  oled.print("IP: ");
  oled.println(WiFi.localIP().toString());
  
  oled.setCursor(0, 35);
  oled.print("MSG: ");
  oled.setCursor(0, 48);
  oled.println(displayText);
  oled.display();
}

// --- 웹 페이지 핸들러 ---
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: 'Inter', sans-serif; background: #0f172a; color: #f8fafc; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }";
  html += ".container { background: #1e293b; padding: 2rem; border-radius: 1.5rem; box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5); width: 90%; max-width: 400px; }";
  html += "h1 { font-size: 1.5rem; margin-bottom: 1.5rem; text-align: center; color: #38bdf8; }";
  html += ".group { margin-bottom: 1.5rem; }";
  html += "label { display: block; margin-bottom: 0.5rem; font-size: 0.875rem; color: #94a3b8; }";
  html += "input[type='text'], select { width: 100%; padding: 0.75rem; border-radius: 0.75rem; border: 1px solid #334155; background: #0f172a; color: white; box-sizing: border-box; }";
  html += "input[type='range'] { width: 100%; cursor: pointer; accent-color: #38bdf8; }";
  html += "button { width: 100%; padding: 0.75rem; border-radius: 0.75rem; border: none; background: #0ea5e9; color: white; font-weight: 600; cursor: pointer; transition: 0.3s; }";
  html += "button:hover { background: #0284c7; }";
  html += "</style></head><body>";
  html += "<div class='container'><h1>Display Dashboard</h1>";
  html += "<form action='/update' method='POST'>";
  
  html += "<div class='group'><label>Display Message</label>";
  html += "<input type='text' name='msg' value='" + displayText + "'></div>";

  html += "<div class='group'><label>Screen Rotation</label>";
  html += "<select name='rot'>";
  html += "<option value='0'" + String(rotationMode==0?" selected":"") + ">0 Degrees</option>";
  html += "<option value='1'" + String(rotationMode==1?" selected":"") + ">90 Degrees</option>";
  html += "<option value='2'" + String(rotationMode==2?" selected":"") + ">180 Degrees</option>";
  html += "<option value='3'" + String(rotationMode==3?" selected":"") + ">270 Degrees</option>";
  html += "</select></div>";
  
  html += "<div class='group'><label>LCD Backlight</label>";
  html += "<input type='range' name='lcd' min='0' max='1023' value='" + String(lcdBrightness) + "'></div>";
  
  html += "<div class='group'><label>OLED Contrast</label>";
  html += "<input type='range' name='oled' min='0' max='255' value='" + String(oledBrightness) + "'></div>";
  
  html += "<button type='submit'>Apply Settings</button></form></div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleUpdate() {
  if (server.hasArg("msg")) displayText = server.arg("msg");
  if (server.hasArg("rot")) rotationMode = server.arg("rot").toInt();
  if (server.hasArg("lcd")) {
    lcdBrightness = server.arg("lcd").toInt();
    analogWrite(2, lcdBrightness); // D4
  }
  if (server.hasArg("oled")) {
    oledBrightness = server.arg("oled").toInt();
    oled.ssd1306_command(SSD1306_SETCONTRAST);
    oled.ssd1306_command(oledBrightness);
  }
  updateDisplays();
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  
  // I2C & OLED 초기화
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println("OLED Failed");
  oled.setRotation(0);
  
  // ST7565 초기화
  u8g2.begin();
  u8g2.setContrast(22);
  
  // LCD 백라이트 핀 설정
  pinMode(2, OUTPUT); // D4
  analogWrite(2, lcdBrightness);

  // WiFi Station 모드 설정
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // OLED에 상태 표시
    oled.clearDisplay();
    oled.setCursor(0, 30);
    oled.println("Connecting WiFi...");
    oled.print("SSID: "); oled.println(ssid);
    oled.display();

    // LCD에 상태 표시
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 30, "Connecting WiFi...");
    u8g2.drawStr(0, 45, ssid);
    u8g2.sendBuffer();
    
    if (millis() > 30000 && WiFi.status() != WL_CONNECTED) {
      Serial.println("\nWiFi Fail - Restarting AP Mode fallback?");
      // 30초 넘게 연결 안되면 알 수 있도록 문구 변경
      u8g2.drawStr(0, 60, "Check Router!");
      u8g2.sendBuffer();
    }
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();

  displayText = "Synced!";
  updateDisplays(); // 여기서 화면을 갱신합니다.
}

void loop() {
  server.handleClient();
}
