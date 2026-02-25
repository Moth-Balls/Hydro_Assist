#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your actual network credentials
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

AsyncWebServer server(80);

// Placeholder sensor values (Later these will come from your Grand Central)
float phValue = 7.0;
float ecValue = 1200.0;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Print the IP address for your tester.html
  Serial.println("\nWiFi connected.");
  Serial.print("Copy this IP into your web interface: ");
  Serial.println(WiFi.localIP());

  // Set up the route that your tester.html "pulls" from
  server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"ph\":" + String(phValue) + ",";
    json += "\"ec\":" + String(ecValue);
    json += "}";
   
    // Add CORS header so your browser doesn't block the request
    AsyncWebHeader* header = new AsyncWebHeader("Access-Control-Allow-Origin", "*");
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.begin();
}

void loop() {
  // Your hydroponics/NFT logic will go here
}