# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Upload

This is an Arduino IDE project. There is no CLI build system.

1. Open the target `.ino` file in Arduino IDE
2. **Tools > Board**: Select the ESP8266 board (NodeMCU 1.0 or Wemos D1 Mini)
3. **Tools > Port**: Select the connected serial port
4. Click **Upload** (Ctrl+U)

To view serial output: **Tools > Serial Monitor** at 115200 baud.

## Project Structure

| File/Folder | Purpose |
| :--- | :--- |
| `ESP8266-MAX7219.ino` | **Main firmware** — 50-animation pixel display system |
| `index.html` | Browser-based simulator — runs identical animation logic in HTML5 Canvas |
| `scanner/scanner.ino` | I2C address scanner utility (scan on SDA=D5/GPIO14, SCL=D6/GPIO12) |
| `ST7565_Test/` | Dual-display test sketch (ST7565 LCD + SSD1306 OLED simultaneously) |
| `ST7565_WebControl/` | WiFi web server for remote control of ST7565 + OLED displays |

## Hardware & Pin Mapping (Main Firmware)

**MAX7219 — SPI**

| MAX7219 | ESP8266 (GPIO) |
| :--- | :--- |
| DIN | GPIO 13 (D7) |
| CS | GPIO 2 (D4) |
| CLK | GPIO 0 (D3) |
| VCC | 5V (Vin) |

`LedControl lc = LedControl(13, 0, 2, 2)` — DIN=13, CLK=0, CS=2, 2 devices (16 digits)

**OLED — I2C (SSD1306)**

| OLED | ESP8266 (GPIO) |
| :--- | :--- |
| SDA | GPIO 14 (D5) |
| SCL | GPIO 12 (D6) |

I2C address: `0x3C`

> Note: The `ST7565_*` sketches use different pin assignments. See those files for their specific wiring.

## Architecture (Main Firmware)

### Display Buffer Pattern
All rendering goes through `byte state[8]` — a software framebuffer for the 8-digit MAX7219. Pixel logic writes to this buffer first, then `drawState()` flushes all 8 rows atomically to prevent tearing. Never write to `lc` directly in animation functions.

### Animation Dispatcher
Animations are registered in a `playlist[50]` array of `AnimRule` structs:
```cpp
struct AnimRule {
    const char* title;
    const char* description;
    void (*func)();
};
```
`loop()` iterates through this array calling each function pointer. To add an animation: implement the function, add an entry to `playlist[]`.

### Timing
All frame timing uses `millis()`-based non-blocking loops — no `delay()` inside animation frames. Each animation runs for exactly **6000ms**:
```cpp
unsigned long t = millis();
while(millis() - t < 6000) { /* frame logic */ }
```

### Segment Bit Layout
Each `state[row]` byte maps to 8 segments (MSB = leftmost segment). Use bitwise operators:
- `|` to turn segments on
- `& ~` to turn segments off
- `^` to toggle
- `<<` / `>>` to shift patterns

### Boot Sequence
`setup()` runs: **AWAKENING** (full brightness warmup, 6s) → **INFO SEQUENCE** (date/time/temp display) → **COUNTDOWN** (8→1 block wipe) → `loop()` starts playlist.

## Dependencies

Install via Arduino IDE Library Manager:

| Library | Author | Used In |
| :--- | :--- | :--- |
| `LedControl` | Eberhard Fahle | Main firmware (MAX7219 SPI) |
| `Adafruit SSD1306` | Adafruit | Main firmware, ST7565_* sketches |
| `Adafruit GFX Library` | Adafruit | Main firmware, ST7565_* sketches |
| `U8g2` | olikraus | ST7565_Test, ST7565_WebControl |
| `ESP8266WiFi` / `ESP8266WebServer` | ESP8266 core | ST7565_WebControl |

ESP8266 board package: install `esp8266` by ESP8266 Community via Arduino Boards Manager.

## Web Simulator

`index.html` is a standalone browser simulator — open directly without a server. It replicates the full 50-animation playlist with identical timing and segment logic using HTML5 Canvas. Use it to preview animation changes before flashing hardware.
