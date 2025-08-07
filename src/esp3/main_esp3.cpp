/**
 * @file main_esp3.cpp
 * @brief Main firmware for the ESP32-S3 Handheld Pendant (ESP3).
 */

#include "lv_conf.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h> // for esp_read_mac()
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <lvgl.h>

#include "config_esp3.h"
#include "shared_structures.h"
#include "persistence_esp3.h"
#include "hmi_handler_esp3.h"
#include "lvgl_driver.h"
#include "ui.h"
#include "communication_esp3.h" // <–– new

extern PendantWebConfig pendant_web_cfg;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

struct_message_from_esp3 outgoing_pendant_data;
struct_message_to_hmi incoming_lcnc_data;

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

static bool sendWithRetry(const struct_message_from_esp3 &msg)
{
    bool ok = communication_esp3_send(msg);
    if (!ok)
    {
        Serial.println("ESP-NOW send failed, retrying once...");
        ok = communication_esp3_send(msg);
    }
    Serial.printf("ESP-NOW send %s\n", ok ? "succeeded" : "failed");
    return ok;
}

static void broadcast_live_status()
{
    uint32_t btn_states;
    int32_t hw_pos;
    uint8_t axis_pos;
    uint8_t step_pos;

    get_pendant_live_status(btn_states, hw_pos, axis_pos, step_pos);

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

static void onWsEvent(AsyncWebSocket * /*server*/,
                      AsyncWebSocketClient *client,
                      AwsEventType type,
                      void * /*arg*/,
                      uint8_t *data,
                      size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        if (DEBUG_ENABLED)
            Serial.printf("WebSocket client #%u connected\n", client->id());

        StaticJsonDocument<4096> doc;
        doc["type"] = "initialConfig";

        // get raw config JSON
        String cfgJson = get_pendant_config_as_json();

        // parse into a temp doc, then inject under "payload"
        StaticJsonDocument<2048> tmp;
        DeserializationError err = deserializeJson(tmp, cfgJson);
        if (err)
        {
            Serial.printf("ERROR: Failed to parse config JSON: %s\n", err.c_str());
        }
        else
        {
            doc["payload"] = tmp.as<JsonObject>();
        }

        String out;
        serializeJson(doc, out);
        client->text(out);
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        if (DEBUG_ENABLED)
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, (char *)data) == DeserializationError::Ok)
        {
            const char *cmd = doc["command"];
            if (strcmp(cmd, "saveConfig") == 0)
            {
                String cfg_out;
                serializeJson(doc["payload"], cfg_out);
                save_pendant_configuration(cfg_out);
                update_hmi_from_config();
            }
            else if (strcmp(cmd, "resetDefaults") == 0)
            {
                reset_pendant_to_defaults();
                update_hmi_from_config();
            }
            else if (strcmp(cmd, "factoryTest") == 0)
            {
                run_factory_test();
            }
            else
            {
                if (DEBUG_ENABLED)
                    Serial.printf("Unknown WS command: %s\n", cmd);
            }
        }
    }
}

static void onEsp3DataReceived(const struct_message_to_hmi &msg)
{
    incoming_lcnc_data = msg;
    update_hmi_from_lcnc(msg);
    broadcast_live_status();
}

//-----------------------------------------------------------------------------
// Arduino setup()
//-----------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    // Mount LittleFS
    if (!LittleFS.begin(true))
    {
        Serial.println("LittleFS mount failed");
        while (true)
            yield();
    }

    // Load config & bind in HMI
    load_pendant_configuration();
    update_hmi_from_config();

    // Init HMI, LVGL driver, UI
    hmi_pendant_init();
    lvgl_driver_init();
    ui_init();

    // Connect Wi-Fi (blocking up to 10s)
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
    {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED)
        Serial.println("\nWi-Fi connected");
    else
        Serial.println("\nWi-Fi failed to connect");

    // Initialize ESP-NOW and callbacks via our communication_esp3 wrapper
    communication_esp3_init();
    communication_esp3_register_receive_callback(onEsp3DataReceived);

    // Send initial HMI snapshot with retry logic
    get_pendant_data(&outgoing_pendant_data);
    sendWithRetry(outgoing_pendant_data);
    broadcast_live_status();

    // Debug: print IP & local MAC
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    uint8_t own_mac[6];
    esp_read_mac(own_mac, ESP_MAC_WIFI_STA);
    Serial.printf("ESP3 MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  own_mac[0], own_mac[1], own_mac[2],
                  own_mac[3], own_mac[4], own_mac[5]);

    // Web server & WebSocket routes
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/index.html", "text/html"); });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/style.css", "text/css"); });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/script.js", "application/javascript"); });
    server.on("/get_config_json", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(200, "application/json", get_pendant_config_as_json()); });
    AsyncElegantOTA.begin(&server);
    server.begin();

    // Print LVGL FPS once after init
    {
        int fps;
        if (lvgl_driver_status(fps))
            Serial.printf("Initial LVGL FPS: %d\n", fps);
    }
}

//-----------------------------------------------------------------------------
// Arduino loop()
//-----------------------------------------------------------------------------
void loop()
{
    // Handle ESP-NOW communication
    communication_esp3_loop();

    // LVGL refresh, HMI tasks, WebSocket housekeeping
    lv_timer_handler();
    hmi_pendant_task();
    ws.cleanupClients();

    // Rate-limit HMI→ESP-NOW with retry
    static unsigned long lastSend = 0;
    const unsigned long interval = 100; // ms
    if (hmi_pendant_data_has_changed() &&
        (millis() - lastSend > interval))
    {
        get_pendant_data(&outgoing_pendant_data);
        sendWithRetry(outgoing_pendant_data);
        broadcast_live_status();
        lastSend = millis();
    }

    // Periodically print LVGL FPS (every 5s)
    static unsigned long lastFpsMs = 0;
    if (millis() - lastFpsMs > 5000)
    {
        int fps;
        if (lvgl_driver_status(fps))
            Serial.printf("LVGL FPS: %d\n", fps);
        lastFpsMs = millis();
    }

    yield();
}
