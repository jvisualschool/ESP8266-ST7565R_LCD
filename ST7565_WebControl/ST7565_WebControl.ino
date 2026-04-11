#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <U8g2lib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// --- 하드웨어 설정 ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 14 // D5 (OLED SDA 고정)
#define OLED_SCL 12 // D6

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_ST7565_ERC12864_F_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/ 0, /* data=*/ 13, /* cs=*/ 16, /* dc=*/ 4, /* reset=*/ 5);

#include "config.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

ESP8266WebServer server(80);

// 전역 상태 변수
String displayText = "HELLO!";
int lcdBrightness = 800;
int oledBrightness = 127;
int rotationMode = 0; // 0, 1, 2, 3 (90도 단위)
int fontSizeIndex = 1; 
int displayMode = 3;   // 기본 모드: 시계 (3)
bool isControlMode = false; // 부팅 시 IP 표시를 위해 false로 시작
unsigned long connectTime = 0; // WiFi 연결 시점 기록용

// 날씨 데이터
String weatherMain = "";
String weatherDesc = "";
float weatherTemp = 0.0;
int weatherHumid = 0;
float weatherWind = 0.0;
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherInterval = 600000; // 10분마다 갱신

// NTP 설정 (대한민국 시간 KST: UTC+9)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 32400, 60000);

// 스톱워치/타이머 변수
unsigned long stopwatchStart = 0;
unsigned long stopwatchElapsed = 0;
bool stopwatchRunning = false;
long timerRemaining = 0; // 초 단위
unsigned long lastTimerUpdate = 0;
bool timerRunning = false;

// 애니메이션 변수
int animType = 0; // 0~9

// --- 애니메이션 함수들 ---
void drawStarfield() {
  for(int i=0; i<20; i++) {
    int x = (millis()/5 + i*13) % 128;
    int y = (i*7) % 64;
    u8g2.drawPixel(x, y);
  }
}

void drawSineWaves() {
  for(int x=0; x<128; x++) {
    int y = 32 + 15 * sin((x + millis()/10) * 0.1);
    u8g2.drawPixel(x, y);
  }
}

void drawBouncingBall() {
  static int bx=64, by=32, bvx=2, bvy=3;
  bx += bvx; by += bvy;
  if(bx<=0 || bx>=127) bvx *= -1;
  if(by<=0 || by>=63) bvy *= -1;
  u8g2.drawDisc(bx, by, 5);
}

void drawExpandingRings() {
  int r = (millis()/20) % 60;
  u8g2.drawCircle(64, 32, r);
  u8g2.drawCircle(64, 32, (r+20)%60);
}

void drawHelix() {
  for(int y=0; y<64; y++) {
    int x1 = 64 + 20 * sin((y + millis()/5) * 0.2);
    int x2 = 64 - 20 * sin((y + millis()/5) * 0.2);
    u8g2.drawPixel(x1, y); u8g2.drawPixel(x2, y);
  }
}

void drawMatrix() {
  for(int i=0; i<10; i++) {
    int x = (i*13);
    int y = (millis()/(5+i) + i*10) % 80 - 10;
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(x, y, "101");
  }
}

void drawSnow() {
  for(int i=0; i<15; i++) {
    int x = (i*20 + i*i) % 128;
    int y = (millis()/(10+i)) % 64;
    u8g2.drawPixel(x,y); u8g2.drawPixel(x+1,y);
  }
}

void drawPlasma() {
  static float t=0; t+=0.1;
  for(int i=0; i<5; i++) {
    int r = 10 + 10*sin(t + i);
    u8g2.drawFrame(64-r*2, 32-r, r*4, r*2);
  }
}

void drawGrid() {
  int offset = (millis()/50)%20;
  for(int i=offset; i<128; i+=20) { u8g2.drawLine(i,0,i,64); }
  for(int i=offset; i<64; i+=20) { u8g2.drawLine(0,i,128,i); }
}

void drawNoise() {
  for(int i=0; i<100; i++) {
    int x = random(0,128); int y = random(0,64);
    u8g2.drawPixel(x,y);
  }
}

void fetchWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    // config.h의 WEATHER_CITY를 사용하도록 수정
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(WEATHER_CITY) + "&appid=" + String(WEATHER_API_KEY) + "&units=metric";
    
    Serial.println("\n[Weather] Fetching update...");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (http.begin(client, url)) {
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
          weatherMain = doc["weather"][0]["main"].as<String>();
          weatherDesc = doc["weather"][0]["description"].as<String>();
          weatherTemp = doc["main"]["temp"];
          weatherHumid = doc["main"]["humidity"];
          weatherWind = doc["wind"]["speed"];
          Serial.printf("[Weather] Success: %.1fC, %s\n", weatherTemp, weatherMain.c_str());
        } else {
          Serial.print("[Weather] JSON Parse Error: ");
          Serial.println(error.c_str());
        }
      } else {
        Serial.printf("[Weather] HTTP Error: %d\n", httpCode);
      }
      http.end();
    }
  } else {
    Serial.println("[Weather] WiFi not connected");
  }
  // 실패하더라도 다음 인터벌까지 대기하여 API 스팸(차단) 방지
  lastWeatherUpdate = millis();
}

void updateLCD() {
  switch(rotationMode) {
    case 0: u8g2.setDisplayRotation(U8G2_R0); break;
    case 1: u8g2.setDisplayRotation(U8G2_R1); break;
    case 2: u8g2.setDisplayRotation(U8G2_R2); break;
    case 3: u8g2.setDisplayRotation(U8G2_R3); break;
  }

  u8g2.clearBuffer();

  // ST7565 LCD Logic
  if (!isControlMode) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 12, "DISPLAY DASHBOARD");
    u8g2.drawLine(0, 15, 128, 15);
    u8g2.drawStr(0, 30, "IP:");
    u8g2.drawStr(25, 30, WiFi.localIP().toString().c_str());
    u8g2.drawStr(0, 45, "MSG:");
    u8g2.drawStr(0, 58, displayText.c_str());
  } else {
    if (displayMode == 0) {
      if (fontSizeIndex == 0) u8g2.setFont(u8g2_font_5x7_tf);
      else if (fontSizeIndex == 1) u8g2.setFont(u8g2_font_helvB08_tr);
      else if (fontSizeIndex == 2) u8g2.setFont(u8g2_font_helvB10_tr);
      else if (fontSizeIndex == 3) u8g2.setFont(u8g2_font_helvB12_tr);
      else if (fontSizeIndex == 4) u8g2.setFont(u8g2_font_logisoso16_tr);
      else if (fontSizeIndex == 5) u8g2.setFont(u8g2_font_logisoso20_tr);
      else if (fontSizeIndex == 6) u8g2.setFont(u8g2_font_logisoso24_tr);
      else if (fontSizeIndex == 7) u8g2.setFont(u8g2_font_logisoso32_tr);
      else if (fontSizeIndex == 8) u8g2.setFont(u8g2_font_logisoso42_tr);
      else if (fontSizeIndex == 9) u8g2.setFont(u8g2_font_logisoso50_tr);
      else u8g2.setFont(u8g2_font_logisoso58_tr);

      int width = u8g2.getStrWidth(displayText.c_str());
      int x, y;
      
      // Y축 위치 계산 (기존 로직 유지)
      switch(fontSizeIndex) {
        case 0: y=35; break; case 1: y=36; break; case 2: y=37; break; case 3: y=38; break; case 4: y=40; break; case 5: y=42; break; case 6: y=44; break; case 7: y=48; break; case 8: y=52; break; case 9: y=56; break; case 10: y=60; break; default: y=44;
      }

      if (width > 128) {
        // 스크롤 로직: 텍스트가 화면보다 길면 왼쪽으로 흐르게 함
        int scrollArea = width + 40; // 텍스트 길이 + 여백
        int offset = (millis() / 30) % scrollArea;
        x = 128 - offset;
        u8g2.drawStr(x, y, displayText.c_str());
        // 연속성을 위해 뒤에 하나 더 그림
        if (x < 0) u8g2.drawStr(x + scrollArea, y, displayText.c_str());
      } else {
        // 기존 중앙 정렬 로직
        x = (128 - width) / 2;
        u8g2.drawStr(x, y, displayText.c_str());
      }
    } else if (displayMode == 1) {
      unsigned long s = millis() / 1000; unsigned long m = s / 60; unsigned long h = m / 60; unsigned long d = h / 24;
      s %= 60; m %= 60; h %= 24;
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 10, "SYSTEM INFORMATION"); u8g2.drawLine(0, 12, 128, 12);
      u8g2.setCursor(0, 25); u8g2.print("CPU Clock: "); u8g2.print(ESP.getCpuFreqMHz()); u8g2.print(" MHz");
      u8g2.setCursor(0, 37); u8g2.print("Flash: "); u8g2.print(ESP.getFlashChipSize()/1024); u8g2.print(" KB");
      u8g2.setCursor(0, 49); u8g2.print("Free Heap: "); u8g2.print(ESP.getFreeHeap()/1024); u8g2.print(" KB");
      u8g2.setCursor(0, 61); u8g2.print("Uptime: "); u8g2.print(d); u8g2.print("d "); u8g2.print(h); u8g2.print("h "); u8g2.print(m); u8g2.print("m "); u8g2.print(s); u8g2.print("s");
    } else if (displayMode == 2) {
      char titleBuf[32];
      sprintf(titleBuf, "%s WEATHER", WEATHER_CITY);
      u8g2.setFont(u8g2_font_6x10_tf); u8g2.drawStr(0, 10, titleBuf); u8g2.drawLine(0, 12, 128, 12);
      u8g2.setFont(u8g2_font_7x14_tf); u8g2.setCursor(0, 28); u8g2.print(weatherTemp, 1); u8g2.print(" C, "); u8g2.print(weatherMain);
      u8g2.setFont(u8g2_font_6x10_tf); u8g2.setCursor(0, 42); u8g2.print("Desc: "); u8g2.print(weatherDesc);
      u8g2.setCursor(0, 52); u8g2.print("Humid: "); u8g2.print(weatherHumid); u8g2.print("%");
      u8g2.setCursor(0, 62); u8g2.print("Wind: "); u8g2.print(weatherWind); u8g2.print(" m/s");
    } else if (displayMode == 3) {
      timeClient.update();
      time_t now = (time_t)timeClient.getEpochTime();
      if (now < 100000) {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(20, 35, "Syncing Time...");
      } else {
        struct tm *ti = localtime(&now);
        int year = ti->tm_year + 1900;
        int month = ti->tm_mon + 1;
        int mday = ti->tm_mday;
        int wday = ti->tm_wday;
        const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        char dateBuf[32];
        sprintf(dateBuf, "%04d-%02d-%02d (%s)", year, month, mday, days[wday]);
        int hour12 = ti->tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* ampm = (ti->tm_hour < 12) ? "AM" : "PM";

        char hBuf[3], mBuf[3], sBuf[3];
        sprintf(hBuf, "%02d", hour12);
        sprintf(mBuf, "%02d", ti->tm_min);
        sprintf(sBuf, "%02d", ti->tm_sec);

        // 상단 날짜
        u8g2.setFont(u8g2_font_6x10_tf); 
        int dw = u8g2.getStrWidth(dateBuf);
        u8g2.drawStr((128 - dw) / 2, 12, dateBuf); 

        // 메인 시계 시작점 (AM/PM 포함 전체 중앙 정렬을 위한 조정)
        int bx = 16; 

        // AM/PM (매우 작은 폰트)
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(bx - 12, 45, ampm);

        // 중앙 시:분 (큰 폰트)
        u8g2.setFont(u8g2_font_logisoso24_tr); 
        u8g2.drawStr(bx,      45, hBuf); // 시
        u8g2.drawStr(bx + 31, 45, ":");  // 첫번째 콜론
        u8g2.drawStr(bx + 40, 45, mBuf); // 분

        // 중앙 초 (작고 가는 폰트)
        u8g2.setFont(u8g2_font_helvR12_tr); 
        u8g2.drawStr(bx + 72, 45, ":");  // 두번째 콜론
        u8g2.drawStr(bx + 78, 45, sBuf); // 초

        // 하단 날씨
        u8g2.setFont(u8g2_font_6x10_tf);
        char weaBuf[32];
        if (weatherMain == "") {
          sprintf(weaBuf, "Loading weather...");
        } else {
          sprintf(weaBuf, "%s: %.1fC, %s", WEATHER_CITY, weatherTemp, weatherMain.c_str());
        }
        int ww = u8g2.getStrWidth(weaBuf);
        u8g2.drawStr((128 - ww) / 2, 62, weaBuf);
      }
    } else if (displayMode == 4) {
      u8g2.setFont(u8g2_font_5x7_tf); u8g2.drawStr(0, 7, "ANIMATION MODE");
      switch(animType) {
        case 0: drawStarfield(); break; case 1: drawSineWaves(); break; case 2: drawBouncingBall(); break; case 3: drawExpandingRings(); break; case 4: drawHelix(); break; case 5: drawMatrix(); break; case 6: drawSnow(); break; case 7: drawPlasma(); break; case 8: drawGrid(); break; case 9: drawNoise(); break;
      }
    }
  }
  u8g2.sendBuffer();
}

void updateOLED() {
  oled.setRotation(rotationMode);
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  
  // OLED Logic - 업타임만 심플하게 표시
  unsigned long totalSec = millis() / 1000;
  unsigned long os = totalSec % 60; unsigned long om = (totalSec / 60) % 60; unsigned long oh = (totalSec / 3600) % 24; unsigned long od = totalSec / 86400;
  oled.setTextSize(1); oled.setCursor(0, 0); oled.println("SYSTEM UPTIME"); oled.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  oled.setTextSize(2); oled.setCursor(0, 25);
  oled.print(od); oled.print("d "); oled.print(oh); oled.print("h"); oled.setCursor(0, 45);
  oled.print(om); oled.print("m "); oled.print(os); oled.print("s");
  oled.display();
}

void handleRoot() {
  if (!isControlMode) { isControlMode = true; updateLCD(); updateOLED(); }
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><style>body{font-family:'Inter',sans-serif;background:#0f172a;color:#f8fafc;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;}.container{background:#1e293b;padding:2rem;border-radius:1.5rem;box-shadow:0 25px 50px -12px rgba(0,0,0,0.5);width:90%;max-width:400px;}h1{font-size:1.5rem;margin-bottom:1.5rem;text-align:center;color:#38bdf8;}.group{margin-bottom:1.2rem;}label{display:block;margin-bottom:0.5rem;font-size:0.875rem;color:#94a3b8;}input[type='text'],select{width:100%;padding:0.75rem;border-radius:0.75rem;border:1px solid #334155;background:#0f172a;color:white;box-sizing:border-box;}input[type='range']{width:100%;cursor:pointer;accent-color:#38bdf8;}button{width:100%;padding:0.75rem;border-radius:0.75rem;border:none;background:#0ea5e9;color:white;font-weight:600;cursor:pointer;transition:0.3s;}button:hover{background:#0284c7;}.row{display:flex;gap:10px;}</style></head><body><div class='container'><h1>Dashboard</h1><form action='/update' method='POST'><div class='group'><label>Mode</label><select name='mode' onchange='fetch(\"/update?mode=\"+this.value).then(()=>location.reload())'>";
  String md[] = {"Message", "System", "Weather", "Clock", "Animation"};
  for(int i=0; i<5; i++) html += "<option value='"+String(i)+"'"+(displayMode==i?" selected":"")+">"+md[i]+"</option>";
  html += "</select></div><div class='row'><div class='group' style='flex:8;'><label>Message</label><input type='text' name='msg' value='"+displayText+"'></div><div class='group' style='flex:2;'><label>Font</label><select name='fsize' onchange='fetch(\"/update?fsize=\"+this.value)'>";
  int sz[] = {6,8,10,12,16,20,24,32,42,50,58};
  for(int i=0; i<11; i++) html += "<option value='"+String(i)+"'"+(fontSizeIndex==i?" selected":"")+">"+String(sz[i])+"</option>";
  html += "</select></div></div><div class='group'><label>Rotation</label><select name='rot'>";
  for(int i=0; i<4; i++) html += "<option value='"+String(i)+"'"+(rotationMode==i?" selected":"")+">"+String(i*90)+"</option>";
  html += "</select></div><div class='group'><label>LCD+OLED Bright</label><input type='range' name='lcd' min='0' max='1023' value='"+String(lcdBrightness)+"' oninput='fetch(\"/update?lcd=\"+this.value)'></div>";

  if(displayMode==4){ html += "<div class='group'><label>Anim</label><select name='atype' onchange='fetch(\"/update?atype=\"+this.value).then(()=>location.reload())'>";
    for(int i=0; i<10; i++) html += "<option value='"+String(i)+"'"+(animType==i?" selected":"")+">Anim "+String(i+1)+"</option>";
    html += "</select></div>";
  }
  html += "<button type='submit'>Apply</button></form></div></body></html>";
  server.send(200, "text/html", html);
}

void handleUpdate() {
  if (server.hasArg("msg")) displayText = server.arg("msg");
  if (server.hasArg("rot")) rotationMode = server.arg("rot").toInt();
  if (server.hasArg("lcd")) { 
    lcdBrightness = server.arg("lcd").toInt(); 
    oledBrightness = map(lcdBrightness, 0, 1023, 0, 255);
    analogWrite(2, lcdBrightness); 
    oled.ssd1306_command(SSD1306_SETCONTRAST);
    oled.ssd1306_command(oledBrightness);
  }
  if (server.hasArg("fsize")) fontSizeIndex = server.arg("fsize").toInt();
  if (server.hasArg("mode")) { displayMode = server.arg("mode").toInt(); if (displayMode == 2) fetchWeather(); }
  if (server.hasArg("atype")) animType = server.arg("atype").toInt();
  updateLCD(); updateOLED(); server.sendHeader("Location", "/"); server.send(303);
}

void handleSW() {
  String c = server.arg("cmd");
  if (c == "toggle") { if (stopwatchRunning) { stopwatchElapsed += (millis() - stopwatchStart); stopwatchRunning = false; } else { stopwatchStart = millis(); stopwatchRunning = true; } }
  else if (c == "reset") { stopwatchElapsed = 0; stopwatchStart = millis(); }
  server.send(200, "text/plain", "ok");
}

void handleTimer() {
  if (server.hasArg("s")) { timerRemaining = server.arg("s").toInt(); timerRunning = true; lastTimerUpdate = millis(); }
  server.send(200, "text/plain", "ok");
}

void setup() {
  // Software SPI 속도 최적화를 위해 빌드 옵션에서 CPU 160MHz 설정 권장
  Serial.begin(115200); Wire.begin(OLED_SDA, OLED_SCL);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  Wire.setClock(400000); // I2C 속도 향상을 통한 OLED 갱신 속도 최적화
  u8g2.begin(); u8g2.setContrast(18); // 대비를 낮추어 잔상(Ghosting) 억제
  pinMode(2, OUTPUT); analogWrite(2, lcdBrightness); WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  server.on("/", handleRoot); server.on("/update", HTTP_POST, handleUpdate); server.on("/update", HTTP_GET, handleUpdate); server.on("/sw", handleSW); server.on("/tm", handleTimer);
  server.begin(); timeClient.begin(); fetchWeather(); updateLCD(); updateOLED();
}

void loop() {
  server.handleClient();
  
  // 부팅 후 5초간 IP 표시 후 자동 모드 전환
  if (!isControlMode && WiFi.status() == WL_CONNECTED) {
    if (connectTime == 0) connectTime = millis();
    if (millis() - connectTime > 5000) {
      isControlMode = true;
      updateLCD(); updateOLED();
    }
  }

  static unsigned long lastLCDRefresh = 0;
  static unsigned long lastOLEDRefresh = 0;

  unsigned long lcdInterval = 1000;
  if (displayMode == 4) lcdInterval = 50;      // 애니메이션 (잔상 고려하여 소폭 하향)
  else if (displayMode == 3) lcdInterval = 1000; // 시계 (초 단위 동기화하여 잔상 최소화)
  else if (displayMode == 1) lcdInterval = 1000; // 시스템 정보

  // LCD 갱신 (빠른 주기)
  if (millis() - lastLCDRefresh >= lcdInterval) {
    updateLCD();
    lastLCDRefresh = millis();
  }

  // OLED 갱신 (1초 주기 - 업타임 갱신용)
  if (millis() - lastOLEDRefresh >= 1000) {
    updateOLED();
    lastOLEDRefresh = millis();
  }

  if (millis() - lastWeatherUpdate > weatherInterval) {
    fetchWeather();
    if (displayMode == 2) { updateLCD(); updateOLED(); }
  }
}
