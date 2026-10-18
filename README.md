# ESP32 OLED Fun Controller

A responsive, self-hosted web control dashboard for an ESP32 connected to an I2C SSD1306 128x64 OLED display.

## Features
- **Live Text Messenger:** Send custom ASCII strings directly from a phone or browser to display on screen.
- **Graphic Emojis:** Built-in vector faces (Smile, Wink, Sad, Surprised, Cool, Heart, Laugh).
- **Smooth Animations (~20 FPS):** Bouncing comet trail, interactive blinking eyes, heartbeat pulse, starfield warp, and sine wave generator.
- **Embedded Mini-Games:** Playable Snake and Chrome-style Dino Run with on-screen D-pad and keyboard controls.
- **Dual Network Fallback:** Connects to existing Wi-Fi station or switches automatically to standalone Soft-AP mode (`ESP32-OLED`).

## Hardware Requirements
- ESP32 Development Board (NodeMCU / DevKit v1)
- 0.96" SSD1306 I2C 128x64 OLED Display
- Breadboard & Jumper Wires

## Wiring Diagram
| OLED Pin | ESP32 Pin |
| :--- | :--- |
| **VCC** | 3.3V / 5V |
| **GND** | GND |
| **SCL** | GPIO 22 |
| **SDA** | GPIO 21 |

## Dependencies
Install via the Arduino Library Manager:
- `Adafruit GFX Library`
- `Adafruit SSD1306`

## Getting Started
1. Clone this repository.
2. Open `ESP32_OLED_Fun.ino` in Arduino IDE.
3. Update `ssid` and `password` with your Wi-Fi details.
4. Select board **ESP32 Dev Module** and upload.
5. Check Serial Monitor (115200 baud) or read the IP address on the OLED to open the dashboard on your phone.

 
