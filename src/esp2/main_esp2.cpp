/**
 * @file main_esp2.cpp
 * @brief Main firmware for ESP2, the Main HMI Panel Controller. (Fully Implemented)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config_esp2.h"
#include "shared_structures.h"
#include "persistence.h"
#include "hmi_handler.h"

// --- GLOBAL OBJECTS ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
struct_message_from_esp2 outgoing_hmi_data;
struct_message_to_hmi incoming_lcnc_data;

// --- HELPER FUNCTIONS ---
void broadcast_live_status()
{
    uint8_t btn_buf[8];
    uint8_t led_buf[8];
    get_live_status_data(btn_buf, led_buf);

    StaticJsonDocument<512> doc;
    doc["type"] = "liveStatus";
    JsonArray buttons = doc["payload"].createNestedArray("buttons");
    for (int i = 0; i < 8; i++)
        buttons.add(btn_buf[i]);
    JsonArray leds = doc["payload"].createNestedArray("leds");
    for (int i = 0; i < 8; i++)
        leds.add(led_buf[i]);

    String json_output;
    serializeJson(doc, json_output);
    ws.textAll(json_output);
}

// --- WEBSOCKET EVENT HANDLER ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        if (DEBUG_ENABLED)
            Serial.printf("WebSocket client #%u connected\n", client->id());

        StaticJsonDocument<4096> doc;
        doc["type"] = "initialConfig";

        // CORRECTED: Parse the config string into a temporary document first.
        StaticJsonDocument<4096> payload_doc;
        deserializeJson(payload_doc, get_config_as_json());

        // Then, assign the parsed object to the "payload" key.
        doc["payload"] = payload_doc;

        String json_output;
        serializeJson(doc, json_output);
        client->text(json_output);
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        if (DEBUG_ENABLED)
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        StaticJsonDocument<4096> doc;
        if (deserializeJson(doc, (char *)data) == DeserializationError::Ok)
        {
            const char *command = doc["command"];
            if (strcmp(command, "saveConfig") == 0)
            {
                String config_payload;
                serializeJson(doc["payload"], config_payload);
                save_configuration(config_payload);
            }
        }
    }
}

// --- ESP-NOW CALLBACKS ---
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    memcpy(&incoming_lcnc_data, incomingData, sizeof(incoming_lcnc_data));
    evaluate_action_bindings(incoming_lcnc_data);
    update_leds_from_lcnc(incoming_lcnc_data);
    broadcast_live_status();
}

// --- MAIN SETUP AND LOOP ---
void setup()
{
    Serial.begin(115200);
    if (!LittleFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
    load_configuration();
    hmi_init();

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    if (DEBUG_ENABLED)
        Serial.printf("WiFi Connected. IP: %s\n", WiFi.localIP().toString().c_str());

    esp_now_init();
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, esp1_mac_address, 6);
    esp_now_add_peer(&peerInfo);

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/index.html", "text/html"); });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/style.css", "text/css"); });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/script.js", "application/javascript"); });
    server.on("/get_config_json", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "application/json", get_config_as_json()); });

    AsyncElegantOTA.begin(&server);
    server.begin();
    if (DEBUG_ENABLED)
        Serial.println("Web server and OTA handler started.");
}

void loop()
{
    hmi_task();
    ws.cleanupClients();
    if (hmi_data_has_changed())
    {
        get_hmi_data(&outgoing_hmi_data);
        esp_now_send(esp1_mac_address, (uint8_t *)&outgoing_hmi_data, sizeof(outgoing_hmi_data));
        broadcast_live_status();
    }
}