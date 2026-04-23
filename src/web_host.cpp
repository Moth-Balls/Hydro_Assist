#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "serial_comm.hpp"
#include "pull_plant.hpp"

// ─── WiFi credentials ────────────────────────────────────────────────────────
const char* ssid     = "isaac-priv-net";
const char* password = ".v7NMA2n4P";

// ─── Latest values (updated every time the Arduino responds) ─────────────────
static float phValue   = 0.0f;
static float ecValue   = 0.0f;
static float tempValue = 0.0f;

// ─── Poll interval: ask the Arduino for data every 2 seconds ─────────────────
static const unsigned long POLL_INTERVAL_MS = 2000;
static unsigned long lastPollMs = 0;

// ─── Non-blocking reader for responses coming back from the Arduino ──────────
static CommReader arduinoReader;

// ─── Pending commands for main task (HTTP handlers run in a separate task) ───
//     AsyncWebServer callbacks run in a different task/ISR context on ESP32,
//     so we must not call Serial2.print() directly from them. Instead we set
//     flags + buffers here and flush from loop().
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

    while (Serial2.available()) Serial2.read(); // flush stale bytes

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

    // ── Serve dashboard HTML ──────────────────────────────────────────
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

    // ── Loading dose endpoint ─────────────────────────────────────────
    //
    // The browser POSTs:
    //   { "gro_ml":15, "micro_ml":10, "bloom_ml":5, "ph_up_ml":2, "ph_dn_ml":0 }
    //
    // We reformat it as an M:2001 message and forward it to the Arduino
    // in loop() (safe Serial2 context) via the pendingDose flag.
    //
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

            // Serialise into the pending buffer and raise the flag.
            // loop() will send it from the main task.
            portENTER_CRITICAL(&doseMux);
            serializeJson(out, pendingDoseJson, sizeof(pendingDoseJson));
            pendingDose = true;
            portEXIT_CRITICAL(&doseMux);
        }
    );

    // ── Start/Stop system-state endpoint ──────────────────────────────
    //
    // The browser POSTs:
    //   { "run": true }   or   { "run": false }
    //
    // We reformat it as an M:2002 message and forward it to the Arduino
    // in loop() via the pendingState flag.
    //
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

    // ── Plant profile endpoint ────────────────────────────────────────
    //
    // The browser POSTs (all EC values in mS/cm):
    //   { "ec_min":0.8, "ec_max":1.2, "ec_avg":1.0,
    //     "ph_min":6.0, "ph_max":7.0, "ph_avg":6.5 }
    //
    // We reformat it as an M:2003 message and forward it to the Arduino
    // in loop() via the pendingProfile flag.
    //
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

    // ── 1. Send any pending loading-dose command to the Arduino ──────
    //       (safe to call Serial2 here — we're in the main Arduino task)
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

    // ── 2. Send any pending system-state command to the Arduino ──────
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

    // ── 2b. Send any pending profile command to the Arduino ───────────
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

    // ── 3. Poll the Arduino for sensor data every POLL_INTERVAL_MS ───
    //
    //  We send {"M":1001} and wait for the Arduino to reply with
    //  {"M":1002,"pH":x,"ec":y,"temp":z} in the next loop iterations.
    //  Because the Arduino responds within microseconds, the reply will
    //  almost always be available by the next loop() iteration.
    //
    if (now - lastPollMs >= POLL_INTERVAL_MS) {
        // Send the sensor-data request
        Serial2.print("{\"M\":1001}\n");
        lastPollMs = now;
    }

    // ── 4. Read and process any incoming bytes from the Arduino ───────
    //
    //  arduinoReader.poll() is non-blocking: it accumulates bytes and
    //  returns true only when a complete '\n'-terminated line is ready.
    //
    if (arduinoReader.poll(Serial2)) {
        const char *line = arduinoReader.buf;

        JsonDocument doc;
        if (!deserializeJson(doc, line)) {

            uint16_t msgType = doc["M"] | 0;

            if (msgType == MSG_SENSOR_RESPONSE) {  // 1002

                phValue   = doc["pH"]   | phValue;
                ecValue   = doc["ec"]   | ecValue;
                tempValue = doc["temp"] | tempValue;

                Serial.printf("<- pH %.2f  EC %.1f  Temp %.2f\n",
                              phValue, ecValue, tempValue);

                // Push to all connected browser clients via SSE
                String payload =
                    "{\"ph\":"   + String(phValue,  2) +
                    ",\"ec\":"   + String(ecValue,   1) +
                    ",\"temp\":" + String(tempValue, 2) + "}";
                events.send(payload.c_str(), "sensor", millis());
            }
        }
    }
}