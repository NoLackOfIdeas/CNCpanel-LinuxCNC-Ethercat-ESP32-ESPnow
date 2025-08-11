/**
 * @file web_interface.cpp
 * @brief Implementation of the Web Server and WebSocket functionality.
 */

#include "web_interface.h"
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config_esp3.h"
#include "persistence_esp3.h"
#include "hmi_handler_esp3.h"

// --- Module-static Globals ---
static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

// --- Private Function Prototypes ---
static void on_ws_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
static void handle_ws_connect(AsyncWebSocketClient *client);
static void handle_ws_data(uint8_t *data, size_t len);

// --- Public API Implementation ---

void web_interface_init()
{
    ws.onEvent(on_ws_event);
    server.addHandler(&ws);

    // Serve static files from LittleFS
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/index.html", "text/html"); });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/style.css", "text/css"); });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/script.js", "application/javascript"); });

    // API endpoint to get current config
    server.on("/get_config_json", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(200, "application/json", get_pendant_config_as_json()); });

    // OTA Updater
    AsyncElegantOTA.begin(&server);

    // Start server
    server.begin();
}

void web_interface_loop()
{
    ws.cleanupClients();
}

void web_interface_broadcast_live_pendant_status(uint32_t btn_states, int32_t hw_pos, uint8_t axis_pos, uint8_t step_pos)
{
    StaticJsonDocument<512> doc;
    doc["type"] = "liveStatus";
    JsonObject payload = doc.createNestedObject("payload");
    payload["buttons"] = btn_states;
    payload["handwheel"] = hw_pos;
    payload["axis"] = axis_pos;
    payload["step"] = step_pos;

    String json_output;
    serializeJson(doc, json_output);
    ws.textAll(json_output);
}

// --- WebSocket Event Handlers ---

static void on_ws_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected\n", client->id());
        handle_ws_connect(client);
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handle_ws_data(data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

static void handle_ws_connect(AsyncWebSocketClient *client)
{
    // On connection, send the entire current configuration
    StaticJsonDocument<2048> doc;
    doc["type"] = "initialConfig";

    // Use a temporary JSON doc to parse the config string before nesting it
    StaticJsonDocument<1536> payload_doc;
    DeserializationError err = deserializeJson(payload_doc, get_pendant_config_as_json());

    if (err)
    {
        Serial.printf("ERROR: Failed to parse config JSON for WS connect: %s\n", err.c_str());
        doc["payload"] = "Error parsing config";
    }
    else
    {
        doc["payload"] = payload_doc.as<JsonObject>();
    }

    String out;
    serializeJson(doc, out);
    client->text(out);
}

static void handle_ws_data(uint8_t *data, size_t len)
{
    StaticJsonDocument<1536> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error)
    {
        Serial.printf("ERROR: WebSocket deserializeJson() failed: %s\n", error.c_str());
        return;
    }

    const char *cmd = doc["command"];
    if (!cmd)
        return;

    if (strcmp(cmd, "saveConfig") == 0)
    {
        String cfg_out;
        serializeJson(doc["payload"], cfg_out);
        save_pendant_configuration(cfg_out);
        update_hmi_from_config(); // Re-apply config to HMI
        Serial.println("INFO: New config saved via WebSocket.");
    }
    else if (strcmp(cmd, "resetDefaults") == 0)
    {
        reset_pendant_to_defaults();
        update_hmi_from_config(); // Re-apply default config to HMI
        Serial.println("INFO: Config reset to defaults via WebSocket.");
    }
    else if (strcmp(cmd, "factoryTest") == 0)
    {
        run_factory_test();
    }
    else
    {
        Serial.printf("WARN: Unknown WebSocket command received: %s\n", cmd);
    }
}