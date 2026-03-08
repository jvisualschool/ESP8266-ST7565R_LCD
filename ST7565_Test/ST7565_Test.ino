#include <Arduino.h>
#include <U8g2lib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 14 // D5
#define OLED_SCL 12 // D6
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ST7565 128x64 LCD 설정 (Software SPI) - R2 = 180도 회전
// CLK을 D3(GPIO0)으로 변경: D5(GPIO14)는 OLED SDA와 충돌함
U8G2_ST7565_ERC12864_F_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/ 0, /* data=*/ 13, /* cs=*/ 16, /* dc=*/ 4, /* reset=*/ 5);

void setup(void) {
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println("OLED Init Failed");
  oled.setRotation(0);
  
  u8g2.begin();
  u8g2.setContrast(22); 
  
  pinMode(2, OUTPUT);    // D4 (GPIO 2)
  digitalWrite(2, HIGH); 
  analogWrite(2, 800);  
}

void loop(void) {
  // --- LCD (u8g2) 그리기 ---
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "Dual Display Test");
  u8g2.drawFrame(0, 20, 128, 44);
  u8g2.drawStr(10, 40, "ST7565 + SSD1306");
  u8g2.drawStr(10, 55, "Success Together!");
  u8g2.sendBuffer();

  // --- OLED (Adafruit) 그리기 ---
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Dual Display Test");
  oled.drawRect(0, 20, 128, 44, SSD1306_WHITE);
  oled.setCursor(10, 32);
  oled.println("ST7565 + SSD1306");
  oled.setCursor(10, 47);
  oled.println("Success Together!");
  oled.display();

  delay(1000);
}
