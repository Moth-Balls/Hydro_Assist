#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// WiFi Credentials
const char* ssid     = "isaac-priv-net";
const char* password = "goodlife";

float phValue = 7.2;
float ecValue = 1250.0;

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);

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

  server.begin();
}

void loop() {
  
  // Simulate sensor changes every 10 seconds
  phValue = 6.5 + ((float)random(-10, 10) / 100.0);
  ecValue = 1100 + random(-20, 20);
  delay(1000);




}