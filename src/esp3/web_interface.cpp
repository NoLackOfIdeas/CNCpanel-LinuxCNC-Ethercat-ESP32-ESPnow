/**
 * @file web_interface.cpp
 * @brief Implementation of the Web Server, REST API, static‐file serving, OTA & WebSocket functionality.
 */

#include "web_interface.h"
#include "config_esp3.h"      // Includes shared_structures.h
#include "persistence_esp3.h" // encoder_inverted, encoder_deadzone, load/save funcs
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "ui.h" // For ui_bridge_apply_config

// --- Module‐static Globals ---
static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

// --- Private Function Prototypes ---
static void on_ws_event(AsyncWebSocket *server,
                        AsyncWebSocketClient *client,
                        AwsEventType type,
                        void *arg,
                        uint8_t *data,
                        size_t len);
static void handle_ws_connect(AsyncWebSocketClient *client);
static void handle_ws_data(uint8_t *data, size_t len);

// --- Public API Implementation ---

void web_interface_init()
{
    // Make sure you've called LittleFS.begin() in setup() before this!

    // WebSocket setup
    ws.onEvent(on_ws_event);
    server.addHandler(&ws);

    // Serve index.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/index.html", "text/html"); });

    // Serve your existing CSS and JS from LittleFS
    server.serveStatic("/style.css", LittleFS, "/style.css", "text/css");
    server.serveStatic("/script.js", LittleFS, "/script.js", "application/javascript");

    // REST endpoint: get full pendant config
    server.on("/get_config_json", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(200, "application/json", get_pendant_config_as_json()); });

    // REST endpoint: read encoder settings
    server.on("/api/encoder", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        StaticJsonDocument<128> doc;
        doc["inverted"] = encoder_inverted;
        doc["deadzone"] = encoder_deadzone;
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out); });

    // REST endpoint: write encoder settings
    server.on("/api/encoder", HTTP_POST, [](AsyncWebServerRequest *req)
              {
        if (!req->hasParam("body", true)) {
            req->send(400, "application/json", "{\"error\":\"no body\"}");
            return;
        }
        String body = req->getParam("body", true)->value();
        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, body) == DeserializationError::Ok) {
            encoder_inverted = doc["inverted"]  | encoder_inverted;
            encoder_deadzone = doc["deadzone"]  | encoder_deadzone;
            save_encoder_config();
            req->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            req->send(400, "application/json", "{\"error\":\"invalid JSON\"}");
        } });

    // OTA
    AsyncElegantOTA.begin(&server);

    // Start the HTTP server
    server.begin();
}

void web_interface_loop()
{
    ws.cleanupClients();
}

void web_interface_broadcast_status(const LcncStatusPacket &data)
{
    StaticJsonDocument<256> doc;
    doc["type"] = "lcncStatus";
    auto payload = doc.createNestedObject("payload");
    payload["feed_override"] = data.feed_override;
    payload["spindle_rpm"] = data.spindle_rpm;
    // …add more fields as needed…

    String out;
    serializeJson(doc, out);
    ws.textAll(out);
}

void web_interface_broadcast_live_pendant_status(uint32_t btns,
                                                 int32_t hw,
                                                 uint8_t axis,
                                                 uint8_t step)
{
    StaticJsonDocument<128> doc;
    doc["type"] = "liveStatus";
    auto payload = doc.createNestedObject("payload");
    payload["buttons"] = btns;
    payload["handwheel"] = hw;
    payload["axis"] = axis;
    payload["step"] = step;

    String out;
    serializeJson(doc, out);
    ws.textAll(out);
}

// --- WebSocket Event Handlers ---

static void on_ws_event(AsyncWebSocket * /*server*/,
                        AsyncWebSocketClient *client,
                        AwsEventType type,
                        void * /*arg*/,
                        uint8_t *data,
                        size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        handle_ws_connect(client);
    }
    else if (type == WS_EVT_DATA)
    {
        handle_ws_data(data, len);
    }
}

static void handle_ws_connect(AsyncWebSocketClient *client)
{
    StaticJsonDocument<512> doc;
    doc["type"] = "initialConfig";

    // embed current pendant configuration
    StaticJsonDocument<512> cfg;
    deserializeJson(cfg, get_pendant_config_as_json());
    doc["payload"] = cfg.as<JsonObject>();

    String out;
    serializeJson(doc, out);
    client->text(out);
}

static void handle_ws_data(uint8_t *data, size_t len)
{
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, data, len) != DeserializationError::Ok)
        return;

    auto cmd = doc["command"].as<const char *>();
    if (!cmd)
        return;

    if (strcmp(cmd, "saveConfig") == 0)
    {
        String cfg_out;
        serializeJson(doc["payload"], cfg_out);
        save_pendant_configuration(cfg_out);
        ui_bridge_apply_config(pendant_web_cfg);
    }
    else if (strcmp(cmd, "resetDefaults") == 0)
    {
        reset_pendant_to_defaults();
    }
}
