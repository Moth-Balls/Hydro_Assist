#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "serial_comm.hpp"
#include "pull_plant.hpp"

// WiFi Credentials
const char* ssid     = "isaac-priv-net";
const char* password = "goodlife";

float phValue = 7.2;
float ecValue = 1250.0;
float tempValue = 25.0;

AsyncWebServer server(80);
AsyncEventSource events("/events");

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  
  while(Serial2.available()) {
    Serial2.read();
  }

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

  // Serve the file from LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/update.html", "text/html");
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"ph\":" + String(phValue, 2) + ",\"ec\":" + String(ecValue, 1) + ",\"temp\":" + String(tempValue, 2) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.addHandler(&events);
  server.begin();
}

void loop() {

  static std::string plant_name = "Arugula";
  static TargetPlant plant = find_plant_data(plant_name);

  if (Serial2.available() > 0) {
    std::array<float, 3> data = read_data(Serial2, Serial);
    if (data[0] != 0.0f || data[1] != 0.0f || data[2] != 0.0f) {
      phValue = data[0];
      ecValue = data[1];
      tempValue = data[2];

      Serial.print("pH: ");
      Serial.print(phValue);
      Serial.print(" | EC: ");
      Serial.print(ecValue);
      Serial.print(" | Temp: ");
      Serial.println(tempValue);

      String payload = "{\"ph\":" + String(phValue, 2) + ",\"ec\":" + String(ecValue, 1) + ",\"temp\":" + String(tempValue, 2) + "}";
      events.send(payload, "sensor", millis());
    }
  }
}