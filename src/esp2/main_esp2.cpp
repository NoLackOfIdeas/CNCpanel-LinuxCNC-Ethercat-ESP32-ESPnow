#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include "config_esp2.h"
#include "shared_structures.h"
// #include "persistence.h" // Auskommentiert, da noch nicht implementiert
#include "hmi_handler.h"

// Globale Objekte
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Datenstrukturen für die Kommunikation
struct_message_to_esp1 outgoing_hmi_data;
struct_message_to_esp2 incoming_lcnc_data;

// Lokale Zustandsvariablen für die Logik-Ebene
uint64_t logical_button_states = 0;

// --- WebSocket Event Handler ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        if (DEBUG_ENABLED)
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        if (DEBUG_ENABLED)
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        // (Hier Logik für Web-Konfiguration einfügen)
    }
}

// --- ESP-NOW Callbacks ---
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    memcpy(&incoming_lcnc_data, incomingData, sizeof(incoming_lcnc_data));
}

// --- Logik-Funktionen ---
void apply_button_logic(uint64_t physical_states)
{
    uint64_t previous_logical_states = logical_button_states;

    for (int i = 0; i < MAX_BUTTONS; i++)
    {
        bool is_physically_pressed = (physical_states >> i) & 1ULL;
        bool rising_edge = is_physically_pressed && !((hmi_get_previous_button_states() >> i) & 1ULL);

        if (rising_edge)
        {
            if (button_configs[i].is_toggle)
            {
                logical_button_states ^= (1ULL << i); // Zustand umkehren
            }
            else if (button_configs[i].radio_group_id > 0)
            {
                for (int j = 0; j < MAX_BUTTONS; j++)
                {
                    if (button_configs[j].radio_group_id == button_configs[i].radio_group_id)
                    {
                        logical_button_states &= ~(1ULL << j);
                    }
                }
                logical_button_states |= (1ULL << i);
            }
        }

        if (!button_configs[i].is_toggle && button_configs[i].radio_group_id == 0)
        {
            if (is_physically_pressed)
                logical_button_states |= (1ULL << i);
            else
                logical_button_states &= ~(1ULL << i);
        }
    }

    if (logical_button_states != previous_logical_states)
    {
        hmi_set_data_changed_flag();
    }
}

void update_final_led_states()
{
    uint64_t final_led_states = 0;
    for (int i = 0; i < MAX_LEDS; i++)
    {
        switch (led_configs[i].binding_type)
        {
        case BOUND_TO_BUTTON:
            if ((logical_button_states >> led_configs[i].bound_button_index) & 1ULL)
            {
                final_led_states |= (1ULL << i);
            }
            break;
        case BOUND_TO_LCNC:
            if ((incoming_lcnc_data.led_states_from_lcnc >> led_configs[i].lcnc_state_bit) & 1ULL)
            {
                final_led_states |= (1ULL << i);
            }
            break;
        case UNBOUND:
            break;
        }
    }
    hmi_set_led_states(final_led_states);
}

void setup()
{
    if (DEBUG_ENABLED)
        Serial.begin(115200);
    // load_configuration(); // Auskommentiert
    hmi_init();

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        if (DEBUG_ENABLED)
            Serial.print(".");
    }
    if (DEBUG_ENABLED)
    {
        Serial.println("\nWiFi connected.");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }

    if (esp_now_init() != ESP_OK)
    { /* Fehlerbehandlung */
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, esp1_mac_address, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    { /* Fehlerbehandlung */
    }

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, (AsyncWebServerRequest * request) { request->send(LittleFS, "/index.html", "text/html"); });
    server.on("/style.css", HTTP_GET, (AsyncWebServerRequest * request) { request->send(LittleFS, "/style.css", "text/css"); });
    server.on("/script.js", HTTP_GET, (AsyncWebServerRequest * request) { request->send(LittleFS, "/script.js", "application/javascript"); });

    AsyncElegantOTA.begin(&server);
    server.begin();
    if (DEBUG_ENABLED)
        Serial.println("HTTP/WebSocket/OTA server started.");
}

void loop()
{
    hmi_task();
    apply_button_logic(hmi_get_button_states());
    update_final_led_states();
    ws.cleanupClients();

    if (hmi_data_has_changed())
    {
        hmi_get_full_hmi_data(&outgoing_hmi_data);
        outgoing_hmi_data.logical_button_states = logical_button_states;
        esp_now_send(esp1_mac_address, (uint8_t *)&outgoing_hmi_data, sizeof(outgoing_hmi_data));
    }
}