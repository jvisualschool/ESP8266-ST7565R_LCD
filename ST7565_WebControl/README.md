# ESP8266 Display Dashboard (ST7565 & SSD1306)

This project transforms an ESP8266 (NodeMCU 1.0) into a comprehensive information and entertainment dashboard, featuring dual-display support (ST7565 LCD + SSD1306 OLED) and a modern responsive Web UI for remote management.

## 🚀 Key Features

### 1. Dual-Display Management
*   **ST7565 LCD (128x64)**: Primary display for interactive content and dashboards.
*   **SSD1306 OLED (128x64)**: Dedicated status display (Permanently shows real-time System Uptime).

### 2. Versatile Display Modes (ST7565)
*   **Clock Mode (Default)**:
    *   Centered Large Digital Clock (NTP synchronized).
    *   Current Date with weekday: `YYYY-MM-DD (Day)`.
    *   Real-time Seoul Weather at the bottom.
*   **Message Mode**:
    *   Custom text display via Web UI.
    *   **11 Font Levels**: From 6px to 58px with automatic vertical centering.
*   **System Info**:
    *   CPU Clock speed, Flash size, and Free Heap monitoring.
    *   **Real-time Uptime**: Accurate to the second (`d h m s`).
*   **Weather Mode**:
    *   Detailed weather for **Seoul, South Korea**.
    *   Shows Temperature (Celsius), Main Condition, Description, Humidity, and Wind Speed.
*   **Animation Mode**:
    *   10 Procedural Animations (Starfield, Sine Waves, Bouncing Ball, Expanding Rings, Helix, Matrix, Snow, Plasma, Grid, Noise).

### 3. Integrated Web UI
*   Modern, responsive design with dark mode.
*   Switch modes and change settings instantly without rebooting.
*   Control LCD backlight (PWM) and OLED brightness.
*   Interactive controls for font sizing and animation selection.

### 4. Smart Boot Sequence
*   Automatically connects to pre-configured WiFi.
*   Shows connection status and **IP address for 5 seconds** upon startup.
*   Transitions automatically to the Clock Mode.

## 🛠 Hardware Configuration

### Pin Mapping
*   **ST7565 LCD (SPI)**:
    *   CLK: `D3` (GPIO0)
    *   Data: `D13` (GPIO13)
    *   CS: `D16` (GPIO16)
    *   DC: `D4` (GPIO2)
    *   Reset: `D5` (GPIO14)
*   **SSD1306 OLED (I2C)**:
    *   SDA: `D5` (GPIO14) - Shared with LCD Reset
    *   SCL: `D6` (GPIO12)
*   **Backlight Control**: `D4` (PWM)

## 📦 Dependencies
*   `U8g2` (v2.35.30+)
*   `Adafruit_GFX` / `Adafruit_SSD1306`
*   `ArduinoJson` (v7.x)
*   `NTPClient` / `WiFiUdp`
*   `ESP8266HTTPClient`

## ⚙️ Configuration
Rename `config.h.example` to `config.h` and update your credentials:
```cpp
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* WEATHER_API_KEY = "YOUR_OPENWEATHERMAP_API_KEY";
const char* WEATHER_CITY = "Seoul,kr";
```

## 📝 License
MIT License
