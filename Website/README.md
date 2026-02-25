# pH/EC Sensor Monitoring System Setup Guide

This system allows you to monitor pH and EC (electrical conductivity) values from an ESP32 connected to an Adafruit Grand Central. 

## System Overview

```
ESP32 Sensor
    ↓ (WiFi - HTTP POST)
Flask API Server (server.py)
    ↓ (HTTP GET)
Web Dashboard (sensor_display.html)
```

## Files Included

1. **sensor_display.html** - Web-based dashboard for viewing real-time sensor data
2. **server.py** - Flask API server to receive and serve sensor data
3. **esp32_sensor_sketch.ino** - Arduino sketch for ESP32 to read and send sensor data
4. **README.md** - This file

## Quick Start

### Step 1: Prepare Your Computer

#### Option A: Using Python (Recommended)

1. Install Python 3.7+ from https://www.python.org/
2. Open PowerShell and install Flask:
   ```powershell
   pip install flask flask-cors
   ```
3. Navigate to the Website folder:
   ```powershell
   cd "C:\Users\bball\OneDrive\Desktop\Website"
   ```
4. Start the server:
   ```powershell
   python server.py
   ```
   The server will start at `http://localhost:5000`

#### Option B: Without Python (Manual Testing)

You can skip the server step and use the "Send Test Data" button in the dashboard for testing without hardware.

### Step 2: Configure ESP32

1. **Install Arduino IDE** from https://www.arduino.cc/en/software
2. **Add ESP32 support:**
   - Go to File → Preferences
   - Add to "Additional Boards Manager URLs": `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Go to Tools → Board Manager
   - Search for "ESP32" and install latest version

3. **Install ArduinoJson Library:**
   - Go to Sketch → Include Library → Manage Libraries
   - Search for "ArduinoJson" by Benoit Blanchon
   - Install version 6.x or latest

4. **Open esp32_sensor_sketch.ino in Arduino IDE**

5. **Configure Settings:**
   ```cpp
   const char* WIFI_SSID = "YOUR_NETWORK_NAME";
   const char* WIFI_PASSWORD = "YOUR_PASSWORD";
   const char* SERVER_URL = "http://YOUR_COMPUTER_IP:5000/api/data";
   ```

   **To find your computer's IP:**
   - Open PowerShell
   - Type: `ipconfig`
   - Look for "IPv4 Address" under your active network (e.g., 192.168.1.100)

6. **Connect ESP32 to your computer via USB**

7. **Upload the sketch:**
   - Select Tools → Board → ESP32
   - Select Tools → Port → COM port for ESP32
   - Click Upload button

8. **Verify in Serial Monitor:**
   - Tools → Serial Monitor
   - Set baud rate to 115200
   - You should see connection messages and sensor readings

### Step 3: Open the Dashboard

1. Open `sensor_display.html` in your web browser
2. In the "API URL" field, enter your server address:
   - Local testing: `http://localhost:5000/api/data`
   - Over network: `http://YOUR_COMPUTER_IP:5000/api/data`
3. Click the "Connect" button
4. You should see:
   - Current pH and EC readings
   - Real-time graphs
   - Data log table

## Sensor Pinout (ESP32)

- **pH Sensor** → GPIO 34 (Analog Pin A0)
- **EC Sensor** → GPIO 35 (Analog Pin A1)
- **GND** → Ground
- **5V** → 5V Power (if needed)

## Sensor Value Conversion

### pH Sensor

Raw ADC Value (0-4095) → Voltage (0-3.3V) → pH (0-14)

**Calibration Process:**
1. Measure voltage output at known pH values (e.g., pH 4, 7, 10)
2. Adjust `PH_OFFSET` and `PH_SCALE` values in the sketch
3. Standard formula: pH = 7 + (voltage - 1.65) / 0.059

### EC Sensor

Raw ADC Value (0-4095) → Voltage (0-3.3V) → EC (0-2000+ µS/cm)

**Calibration Process:**
1. Measure voltage output with known conductivity solutions
2. Adjust `EC_OFFSET` and `EC_SCALE` values
3. Most EC sensors: 3.3V = maximum conductivity ~2000 µS/cm

## Dashboard Features

### Display Elements
- **Real-time Readings**: Current pH and EC values with timestamps
- **Historical Graphs**: 2 charts tracking pH and EC over time (up to 100 data points)
- **Data Log**: Table of last 20 readings with timestamps

### Controls
- **Connect**: Initialize connection to API server
- **Send Test Data**: Add random test data (no hardware needed)
- **Toggle Auto-Refresh**: Enable/disable automatic updates every 2 seconds
- **Clear Data**: Remove all stored data and reset display

### Connection Status
Green indicator = Connected to server
Red indicator = Server unreachable

## API Endpoints

### GET /api/data
Returns the latest sensor reading

**Response:**
```json
{
  "ph": 7.25,
  "ec": 1500,
  "timestamp": "2024-02-11T10:30:45.123456"
}
```

### POST /api/data
Receive new sensor data from ESP32

**Request Body:**
```json
{
  "ph": 7.25,
  "ec": 1500
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Data received",
  "data": {
    "ph": 7.25,
    "ec": 1500,
    "timestamp": "2024-02-11T10:30:45.123456"
  }
}
```

### GET /api/history
Returns all stored historical data (up to 1000 readings)

### POST /api/clear-history
Clears all historical data on the server

## Troubleshooting

### WiFi Connection Issues
- Verify WIFI_SSID and WIFI_PASSWORD are correct
- Check if your ESP32 is on the same network as computer
- Look at Serial Monitor for connection messages

### Can't Find Server
- Confirm server is running (`python server.py`)
- Check SERVER_URL IP address matches your computer's IP
- Ensure ESP32 can reach the IP (ping from ESP32 won't work, but HTTP will)
- Check Windows Firewall isn't blocking port 5000

### Sensor Readings Are Wrong
- Verify sensor connections (correct GPIO pins)
- Check sensor calibration values
- Verify ADC is reading reasonable values (Serial Monitor output)
- Compare with known pH/EC solutions

### Dashboard Not Updating
- Check browser console for errors (F12)
- Verify API URL is correct
- Look for messages in the dashboard (colored boxes at top)
- Check server logs in PowerShell/Terminal

## Manual Testing Without Hardware

1. Open `sensor_display.html` in browser
2. DON'T click Connect (no need for server)
3. Click "Send Test Data" to add sample readings
4. Watch the graphs and table update in real-time
5. Perfect for testing the interface!

## Advanced Usage

### Changing Update Frequency

In **server.py**, keep at 2000ms (2 seconds) for stability.

In **esp32_sensor_sketch.ino**, change this line:
```cpp
const int UPLOAD_INTERVAL = 5000; // Change to desired milliseconds
```

### Storing More Data Points

In **sensor_display.html**, change:
```javascript
const MAX_DATA_POINTS = 100; // Change to desired number
```

In **server.py**, change:
```python
MAX_HISTORY = 1000 # Change to desired number
```

### Adding More Sensors

1. Add new GPIO pin constant in sketch:
   ```cpp
   const int NEW_SENSOR_PIN = 32;
   ```

2. Add read function:
   ```cpp
   float readNewSensor() {
     // Add reading logic
   }
   ```

3. Add to JSON payload:
   ```cpp
   doc["new_value"] = readNewSensor();
   ```

4. Update HTML to display new value

## Support & Resources

- **ESP32 Documentation**: https://docs.espressif.com/
- **Arduino IDE Help**: https://www.arduino.cc/en/Guide
- **Flask Documentation**: https://flask.palletsprojects.com/
- **Chart.js (Graphing)**: https://www.chartjs.org/

## License

This project is provided as-is for educational and personal use.

---

**Last Updated**: February 11, 2026
**System**: pH/EC Sensor Monitor v1.0
