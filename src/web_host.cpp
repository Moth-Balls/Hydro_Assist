#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// WiFi Credentials
const char* ssid     = "isaac-priv-net";
const char* password = "goodlife";

float phValue = 7.2;
float ecValue = 1250.0;

AsyncWebServer server(80);
AsyncEventSource events("/events");

void setup() {
  Serial.begin(115200);
  
  // Initialize Serial2 for GC M4 data at 9600 baud to match Grand Central
  Serial2.begin(9600);
  
  // Flush any initial junk in the buffer
  while(Serial2.available()) {
    Serial2.read();
  }

  // initialise filesystem before serving anything
  if(!LittleFS.begin()){
    Serial.println("LittleFS mount failed!");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected! IP: ");
  Serial.println(WiFi.localIP());

  // serve the file from LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/update.html", "text/html");
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"ph\":" + String(phValue) + ",\"ec\":" + String(ecValue) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.addHandler(&events);
  server.begin();
}

void loop() {
  static String inputString = "";

  // Continuously read without blocking
  while (Serial2.available()) {
    char c = Serial2.read();
    
    // Only mirror printable ASCII, newlines, and carriage returns to avoid garbled text
    if ((c >= 32 && c <= 126) || c == '\n' || c == '\r') {
      Serial.write(c); 
    }
    
    if (c == '\n') {
      // Don't try to parse empty lines
      if (inputString.length() > 0) {
        JsonDocument doc;
        // Check if parsing succeeds without exceptions
        DeserializationError error = deserializeJson(doc, inputString);
        if (!error) {
          if (doc.containsKey("ph")) phValue = doc["ph"];
          if (doc.containsKey("ec")) ecValue = doc["ec"];
        } else {
          Serial.print("JSON Parse Error: ");
          Serial.println(error.c_str());
        }
      }
      inputString = "";
    } else if (c != '\r' && c >= 32 && c <= 126) {
      // Only append valid readable characters to our JSON string
      // This prevents hidden control characters from crashing ArduinoJson
      inputString += c;
    }
  }
}