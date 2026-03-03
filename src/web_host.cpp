#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// WiFi Credentials
const char* ssid = "isaac-priv-net";
const char* password = "goodlife";

float phValue = 7.2;
float ecValue = 1250.0;

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("An error occurred while mounting LittleFS");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.print("\nConnected! IP: "); 
  Serial.println(WiFi.localIP());

  // Serve the HTML file from LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/website.html", "text/html");
  });

  // Serve the JSON data
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"ph\":" + String(phValue) + ",\"ec\":" + String(ecValue) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.begin();
}

void loop() {
  // Simulating sensor changes every 10 seconds
  phValue = 6.5 + ((float)random(-10, 10) / 100.0);
  ecValue = 1100 + random(-20, 20);
  delay(10000); 
}