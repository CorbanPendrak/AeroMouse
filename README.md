# AeroMouse - Your Computer, Un-tethered 🖱️✈️

> A wireless air mouse that translates natural hand gestures into precise cursor control. No drivers. No hassle. Just pure freedom!

[![KnightHacks VIII](https://img.shields.io/badge/KnightHacks-VIII-orange?style=for-the-badge)](https://2025.knighthacks.org)
[![Demo Video](https://img.shields.io/badge/Demo-YouTube-red?style=for-the-badge)](https://www.youtube.com/watch?v=fM6QrrNmMAM)
[![Devpost](https://img.shields.io/badge/Devpost-Project-blue?style=for-the-badge)](https://devpost.com/software/floating-mouse)

## 🎯 Problem Statement

The traditional mouse has been chained to the desk for 40 years! We're physically tied to flat surfaces, which is:
- **Restrictive**: Awkward on couches, beds, or small spaces
- **Un-ergonomic**: Leads to wrist strain and RSI from repetitive use  
- **Clunky**: Presenters stuck behind podiums or need separate "clicker" devices

## ✨ Solution

AeroMouse uses advanced sensor fusion and gesture recognition to translate your hand movements into precise cursor control - **no desk required!**

### Key Features
- 🎯 **Precision Tracking**: Advanced sensor fusion for smooth, accurate cursor movement
- 📱 **Wireless Freedom**: Bluetooth connectivity - no dongles or drivers needed
- ⚡ **Long Battery Life**: Optimized power consumption for extended use
- 🔄 **Drag & Drop**: Hold buttons while tracking to drag files and select text
- 📜 **Scroll Mode**: Double-tap middle button to switch to gentle scrolling
- 🎚️ **Precision Control**: Hold both buttons for ultra-precise cursor positioning
- 🔧 **Auto-Calibration**: 5 quick taps to recalibrate on the fly

## 🛠️ Hardware Requirements

### Components
- **ESP32 DevKit** - Main microcontroller
- **MPU6050** - 6-axis IMU sensor (accelerometer + gyroscope)  
- **3x Push Buttons** - Left, right, and middle mouse buttons
- **Breadboard & Jumper Wires** - For prototyping
- **[OPTIONAL]** Battery pack for portable operation

### Wiring Diagram
```
ESP32    →    MPU6050
-----    →    -------
21 (SDA) →    SDA
22 (SCL) →    SCL  
3.3V     →    VCC
GND      →    GND

ESP32    →    Buttons
-----    →    -------
16       →    LEFT_BUTTON   (with pull-up resistor)
19       →    RIGHT_BUTTON  (with pull-up resistor)
17       →    MIDDLE_BUTTON (with pull-up resistor)
```

**TODO**: Add actual circuit diagram image once finalized

## 🚀 Installation & Setup

### 1. Arduino IDE Setup
1. Install Arduino IDE 2x or later
2. Add ESP32 board support:
   - Go to **File → Preferences**
   - Add to Additional Board Manager URLs:
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - Go to **Tools → Board → Boards Manager**
   - Search and install "ESP32"

### 2. Install Required Libraries
Install these libraries via **Tools → Manage Libraries**:
- `Adafruit MPU6050` by Adafruit
- `Adafruit Sensor` by Adafruit

Install the our customized `ESP32 BLE Mouse` (original by T-vK) with **Sketch → Include Libraries → Add .ZIP Library.**

### 3. Upload Code
1. Clone this repository:
   ```bash
   git clone https://github.com/CorbanPendrak/Air-Mouse.git
   cd Air-Mouse
   ```
2. Open `main/main.ino` in Arduino IDE
3. Select your ESP32 board and port
4. Upload the code

### 4. Pairing with Your Computer
1. Power on the AeroMouse
2. On your computer, go to Bluetooth settings
3. Look for device named **"Air Mouse"** 
4. Pair and connect

## 🎮 Usage Guide

### Basic Controls
- **Hold middle button**: Activate cursor tracking
- **Tilt up/down**: Move cursor vertically  
- **Twist left/right**: Move cursor horizontally
- **Press left/right buttons**: Mouse clicks

### Advanced Features
- **Double-tap middle button**: Toggle scroll mode
- **5 quick taps middle button**: Recalibrate sensors
- **Long-press right button (2s)**: Cycle sensitivity (Low/Medium/High)

### Drag & Drop
- **Hold left button** → **Press middle button** → **Drag with left down**
- **Hold right button** → **Press middle button** → **Drag with right down**

## 🧪 Technical Implementation

### Sensor Fusion Algorithm
- **Complementary Filter**: Combines gyroscope and accelerometer data (96% gyro, 4% accel)
- **Bias Correction**: Adaptive gyro bias removal for drift compensation
- **Momentum System**: Captures slow, continuous movements with 0.88 decay rate

### Movement Processing
- **Yaw-based Horizontal**: Uses Z-axis rotation for left/right (orientation-independent)
- **Pitch-based Vertical**: Uses X-axis rotation for up/down movement
- **Non-linear Response Curves**: Separate curves for cursor and scroll modes
- **Exponential Smoothing**: Reduces jitter while maintaining responsiveness

### Power Management
- **Connection Monitoring**: Reduces update rate when disconnected
- **Sensor Health Checks**: Automatic I2C recovery on sensor failures
- **Optimized Bluetooth**: Efficient HID protocol implementation

## 🏆 Demo & Recognition

- 🎥 **[Demo Video](https://www.youtube.com/watch?v=fM6QrrNmMAM)** - See AeroMouse in action!
- 🏅 **[Devpost Submission](https://devpost.com/software/floating-mouse)** - Full project details
- 🎪 **[KnightHacks VIII](https://knighthacks.org/)** - University of Central Florida hackathon club

## 🤝 Contributing

We welcome contributions! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## 🐛 Troubleshooting

### Common Issues
- **Device won't pair**: Check Bluetooth is enabled, try restarting ESP32
- **Cursor drift**: Perform 5-tap recalibration, ensure device is stationary during startup
- **Erratic movement**: Check wiring connections, verify MPU6050 mounting is secure
- **No response**: Check serial monitor for error messages, verify power supply

### Serial Debug Output
The device provides helpful debug information via Serial (115200 baud):
- Connection status updates
- Calibration confirmation  
- Mode change notifications
- Error detection and recovery

## 📝 License

This code uses the GNU AGPLv3 license, which means that you are free to modify the code and use it commercially, but you must disclose the source and license modifications under the same license. Check the LICENSE file for details. 

## 👥 Team

Team Roles & Contributions
- **Adam Stiles**:
  Implemented core button input handling for clicks and mode triggers; collaborated on integration points with the mouse workflow.

- **Sebastian Striba**:
  Developed the project website and presentation materials; structured visuals and copy for clear demos and documentation.

- **Ryan Schmidt**:
  Assisted with button logic and initial gyroscope hookup; supported general debugging across modules.

- **Corban Pendrak** (Team Lead):
  Coordinated planning and scope; originated the concept; implemented Bluetooth HID mouse configuration and gyroscope integration; deployed GitHub Pages and revised the front-end site; facilitated code integration and team collaboration.

- AI Assistance:
  Ensured reliable behavior and mode-action consistency through generated suggestions used during development.

## 🙏 Acknowledgments

- **KnightHacks VIII** organizers and sponsors
- **Adafruit** for excellent sensor libraries
- **T-vK** for the ESP32 BLE Mouse library
- **University of Central Florida** for hosting the hackathon

---

Made with ❤️ for KnightHacks VIII  
*Breaking free from desktop tyranny, one gesture at a time!*
