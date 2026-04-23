#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "serial_comm.hpp"
#include "pull_plant.hpp"

// WiFi stuff
const char* ssid     = "fake-net"; // Replace with actual SSID
const char* password = "password"; // Replace with actual password

static float phValue   = 0.0f;
static float ecValue   = 0.0f;
static float tempValue = 0.0f;

static const unsigned long POLL_INTERVAL_MS = 2000;
static unsigned long lastPollMs = 0;

static CommReader mcuReader;

static volatile bool   pendingDose    = false;
static char            pendingDoseJson[256] = {};

static volatile bool   pendingState    = false;
static char            pendingStateJson[128] = {};

static volatile bool   pendingProfile  = false;
static char            pendingProfileJson[192] = {};

static portMUX_TYPE    doseMux = portMUX_INITIALIZER_UNLOCKED;

AsyncWebServer server(80);
AsyncEventSource events("/events");


void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);

    while (Serial2.available()) Serial2.read(); // flush bytes

    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed!");
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\nConnected! IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/website.html", "text/html");
    });

    // ── Snapshot endpoint (last known values, no blocking Serial call) ─
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{\"ph\":"   + String(phValue,   2) +
                      ",\"ec\":"   + String(ecValue,    1) +
                      ",\"temp\":" + String(tempValue,  2) + "}";
        AsyncWebServerResponse *r = request->beginResponse(200, "application/json", json);
        r->addHeader("Access-Control-Allow-Origin", "*");
        request->send(r);
    });

    server.on("/loading-dose", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            String resp = "{\"ok\":true}";
            AsyncWebServerResponse *r = request->beginResponse(200, "application/json", resp);
            r->addHeader("Access-Control-Allow-Origin", "*");
            request->send(r);
        },
        nullptr,
        [](AsyncWebServerRequest *request,
           uint8_t *data, size_t len, size_t index, size_t total) {

            static char bodyBuf[256];
            static size_t bodyLen = 0;

            if (index == 0) bodyLen = 0;

            size_t space = sizeof(bodyBuf) - bodyLen - 1;
            size_t copy  = (len < space) ? len : space;
            memcpy(bodyBuf + bodyLen, data, copy);
            bodyLen += copy;
            bodyBuf[bodyLen] = '\0';

            if (index + len < total) return; // wait for more chunks

            JsonDocument in;
            if (deserializeJson(in, bodyBuf, bodyLen)) return;

            // Build M:2001 forwarding message
            JsonDocument out;
            out["M"]      = MSG_LOAD_DOSE;
            out["gro"]    = in["gro_ml"]   | 0.0f;
            out["micro"]  = in["micro_ml"] | 0.0f;
            out["bloom"]  = in["bloom_ml"] | 0.0f;
            out["ph_up"]  = in["ph_up_ml"] | 0.0f;
            out["ph_dn"]  = in["ph_dn_ml"] | 0.0f;

            portENTER_CRITICAL(&doseMux);
            serializeJson(out, pendingDoseJson, sizeof(pendingDoseJson));
            pendingDose = true;
            portEXIT_CRITICAL(&doseMux);
        }
    );

    server.on("/set-state", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            String resp = "{\"ok\":true}";
            AsyncWebServerResponse *r = request->beginResponse(200, "application/json", resp);
            r->addHeader("Access-Control-Allow-Origin", "*");
            request->send(r);
        },
        nullptr,
        [](AsyncWebServerRequest *request,
           uint8_t *data, size_t len, size_t index, size_t total) {

            static char bodyBuf[128];
            static size_t bodyLen = 0;

            if (index == 0) bodyLen = 0;

            size_t space = sizeof(bodyBuf) - bodyLen - 1;
            size_t copy  = (len < space) ? len : space;
            memcpy(bodyBuf + bodyLen, data, copy);
            bodyLen += copy;
            bodyBuf[bodyLen] = '\0';

            if (index + len < total) return; // wait for more chunks

            JsonDocument in;
            if (deserializeJson(in, bodyBuf, bodyLen)) return;

            // Build M:2002 forwarding message
            JsonDocument out;
            out["M"]   = MSG_SYSTEM_STATE;
            out["run"] = in["run"] | false;

            portENTER_CRITICAL(&doseMux);
            serializeJson(out, pendingStateJson, sizeof(pendingStateJson));
            pendingState = true;
            portEXIT_CRITICAL(&doseMux);
        }
    );

    server.on("/set-profile", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            String resp = "{\"ok\":true}";
            AsyncWebServerResponse *r = request->beginResponse(200, "application/json", resp);
            r->addHeader("Access-Control-Allow-Origin", "*");
            request->send(r);
        },
        nullptr,
        [](AsyncWebServerRequest *request,
           uint8_t *data, size_t len, size_t index, size_t total) {

            static char bodyBuf[192];
            static size_t bodyLen = 0;

            if (index == 0) bodyLen = 0;

            size_t space = sizeof(bodyBuf) - bodyLen - 1;
            size_t copy  = (len < space) ? len : space;
            memcpy(bodyBuf + bodyLen, data, copy);
            bodyLen += copy;
            bodyBuf[bodyLen] = '\0';

            if (index + len < total) return; // wait for more chunks

            JsonDocument in;
            if (deserializeJson(in, bodyBuf, bodyLen)) return;

            // Build M:2003 forwarding message
            JsonDocument out;
            out["M"]      = MSG_SET_PROFILE;
            out["ec_min"] = in["ec_min"] | 0.0f;
            out["ec_max"] = in["ec_max"] | 0.0f;
            out["ec_avg"] = in["ec_avg"] | 0.0f;
            out["ph_min"] = in["ph_min"] | 0.0f;
            out["ph_max"] = in["ph_max"] | 0.0f;
            out["ph_avg"] = in["ph_avg"] | 0.0f;

            portENTER_CRITICAL(&doseMux);
            serializeJson(out, pendingProfileJson, sizeof(pendingProfileJson));
            pendingProfile = true;
            portEXIT_CRITICAL(&doseMux);
        }
    );

    server.addHandler(&events);
    server.begin();

    Serial.println("Web server started.");
}


void loop() {

    unsigned long now = millis();

    portENTER_CRITICAL(&doseMux);
    bool hasDose = pendingDose;
    if (hasDose) pendingDose = false;
    portEXIT_CRITICAL(&doseMux);

    if (hasDose) {
        Serial2.print(pendingDoseJson);
        Serial2.print('\n');
        Serial.print("Forwarded loading dose -> ");
        Serial.println(pendingDoseJson);
    }

    portENTER_CRITICAL(&doseMux);
    bool hasState = pendingState;
    if (hasState) pendingState = false;
    portEXIT_CRITICAL(&doseMux);

    if (hasState) {
        Serial2.print(pendingStateJson);
        Serial2.print('\n');
        Serial.print("Forwarded system state -> ");
        Serial.println(pendingStateJson);
    }

    portENTER_CRITICAL(&doseMux);
    bool hasProfile = pendingProfile;
    if (hasProfile) pendingProfile = false;
    portEXIT_CRITICAL(&doseMux);

    if (hasProfile) {
        Serial2.print(pendingProfileJson);
        Serial2.print('\n');
        Serial.print("Forwarded profile -> ");
        Serial.println(pendingProfileJson);
    }

    if (now - lastPollMs >= POLL_INTERVAL_MS) {
        Serial2.print("{\"M\":1001}\n");
        lastPollMs = now;
    }

    if (mcuReader.poll(Serial2)) {
        const char *line = mcuReader.buf;

        JsonDocument doc;
        if (!deserializeJson(doc, line)) {

            uint16_t msgType = doc["M"] | 0;

            if (msgType == MSG_SENSOR_RESPONSE) {  // 1002

                phValue   = doc["pH"]   | phValue;
                ecValue   = doc["ec"]   | ecValue;
                tempValue = doc["temp"] | tempValue;

                Serial.printf("<- pH %.2f  EC %.1f  Temp %.2f\n",
                              phValue, ecValue, tempValue);

                String payload =
                    "{\"ph\":"   + String(phValue,  2) +
                    ",\"ec\":"   + String(ecValue,   1) +
                    ",\"temp\":" + String(tempValue, 2) + "}";
                events.send(payload.c_str(), "sensor", millis());
            }
        }
    }
}