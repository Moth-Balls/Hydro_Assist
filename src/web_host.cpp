/*
  ESP32 pH/EC Sensor Data Logger
  
  This sketch reads pH and EC sensor values and sends them to a Flask API server
  
  Hardware Setup:
  - Connect pH sensor to analog pin A0
  - Connect EC sensor to analog pin A1
  - ESP32 connected to WiFi
  
  Dependencies:
  - ESP32 Board by Espressif Systems (installed via Arduino IDE)
  - ArduinoJson library (install via Library Manager)
  
  Configuration:
  1. Update WIFI_SSID and WIFI_PASSWORD
  2. Update SERVER_URL with your server's IP address
  3. Calibrate pH and EC sensors
*/

// #include <WiFi.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Server configuration
const char* SERVER_URL = "http://192.168.1.100:5000/api/data";  // Update with your computer's IP

// Sensor pins
const int PH_PIN = 34;      // Analog pin for pH sensor
const int EC_PIN = 35;      // Analog pin for EC sensor

// Sensor calibration values (adjust based on your sensors)
const float PH_OFFSET = 0.0;      // pH offset calibration
const float PH_SCALE = 1.0;       // pH scale calibration
const float EC_OFFSET = 0.0;      // EC offset calibration
const float EC_SCALE = 1.0;       // EC scale calibration

// Sampling
const int SAMPLE_COUNT = 10;
const int SAMPLE_DELAY = 100;     // ms between samples
const int UPLOAD_INTERVAL = 5000; // Send data every 5 seconds

unsigned long lastUploadTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\nStarting pH/EC Sensor Logger...");
  
  // Configure ADC
  analogReadResolution(12);
  
  // Connect to WiFi
  connectToWiFi();
  
  Serial.println("Setup complete. Starting measurements...");
}

void loop() {
  // Check if it's time to send data
  if (millis() - lastUploadTime > UPLOAD_INTERVAL) {
    // Read sensors
    float ph = readPHSensor();
    float ec = readECSensor();
    
    // Display readings
    Serial.print("pH: ");
    Serial.print(ph, 2);
    Serial.print(" | EC: ");
    Serial.print(ec, 0);
    Serial.println(" µS/cm");
    
    // Send to server
    if (WiFi.status() == WL_CONNECTED) {
      sendToServer(ph, ec);
    } else {
      Serial.println("WiFi disconnected. Reconnecting...");
      connectToWiFi();
    }
    
    lastUploadTime = millis();
  }
  
  delay(100);
}

/*
  Read pH sensor value
  Typical analog readings: 0-4095 for 0-3.3V
  Standard pH sensor: ~59mV per pH unit
*/
float readPHSensor() {
  long sumValue = 0;
  
  // Take multiple samples
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sumValue += analogRead(PH_PIN);
    delay(SAMPLE_DELAY);
  }
  
  // Average
  float averageValue = (float)sumValue / SAMPLE_COUNT;
  
  // Convert analog reading to voltage (0-3.3V)
  float voltage = (averageValue / 4095.0) * 3.3;
  
  // Convert voltage to pH (calibration required)
  // Standard formula: pH = 7 + (voltage - offset) / slope
  // slope is typically around 59mV per pH unit
  float ph = 7.0 + (voltage - 1.65) / 0.059;
  
  // Apply calibration
  ph = ph * PH_SCALE + PH_OFFSET;
  
  // Clamp to valid pH range
  if (ph < 0) ph = 0;
  if (ph > 14) ph = 14;
  
  return ph;
}

/*
  Read EC (Electrical Conductivity) sensor value
  EC sensors typically output analog signal proportional to conductivity
  Most EC sensors: 0V = 0 µS/cm, 3V = max value (usually 2000-5000 µS/cm)
*/
float readECSensor() {
  long sumValue = 0;
  
  // Take multiple samples
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sumValue += analogRead(EC_PIN);
    delay(SAMPLE_DELAY);
  }
  
  // Average
  float averageValue = (float)sumValue / SAMPLE_COUNT;
  
  // Convert analog reading to voltage (0-3.3V)
  float voltage = (averageValue / 4095.0) * 3.3;
  
  // Convert voltage to EC
  // Assuming 3.3V = 2000 µS/cm (adjust based on your sensor)
  float ec = (voltage / 3.3) * 2000.0;
  
  // Apply calibration
  ec = ec * EC_SCALE + EC_OFFSET;
  
  if (ec < 0) ec = 0;
  
  return ec;
}

/*
  Connect to WiFi network
*/
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

/*
  Send sensor data to server via HTTP POST
*/
void sendToServer(float ph, float ec) {
  HTTPClient http;
  
  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["ph"] = ph;
  doc["ec"] = ec;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send POST request
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Server response: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error sending data. Code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}


/* ===== CONFIGURATION GUIDE =====

### WiFi Setup:
1. Update WIFI_SSID and WIFI_PASSWORD with your network credentials

### Server Setup:
1. Update SERVER_URL with your computer's IP address
   - Find your IP: 
     - Windows: Open Command Prompt, type "ipconfig", look for IPv4 Address
     - Mac/Linux: Open Terminal, type "ifconfig"
   - Example: http://192.168.1.100:5000/api/data

### Sensor Calibration:
1. pH Sensor Calibration:
   - Measure pH of known solutions (e.g., pH 4, 7, 10)
   - Adjust PH_OFFSET and PH_SCALE until readings match known values
   
2. EC Sensor Calibration:
   - Measure EC of known conductivity solutions
   - Adjust EC_OFFSET and EC_SCALE until readings match known values

### Testing:
1. Upload this sketch to your ESP32
2. Open Serial Monitor (Tools > Serial Monitor, 115200 baud)
3. Start the Python server: python server.py
4. Open sensor_display.html in browser
5. Set API URL to: http://computer-ip:5000/api/data
6. Click "Connect"

*/