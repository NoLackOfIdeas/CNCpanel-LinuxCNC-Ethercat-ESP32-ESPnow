/**
 * @file main_esp3.cpp
 * @brief Main firmware for the ESP32-S3 Handheld Pendant.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <lvgl.h>

// --- Core Application Headers ---
#include "config_esp3.h" // Master config/include file
#include "persistence_esp3.h"
#include "hmi_handler_esp3.h"
#include "communication_esp3.h"
#include "web_interface.h"
#include "lvgl_driver.h"
#include "ui.h"
#include "ui_popups.h"
#include "ui_tab_logic.h"

// --- Constants ---
static constexpr unsigned long PENDANT_SEND_INTERVAL_MS = 50;
static constexpr unsigned long STATUS_BROADCAST_INTERVAL_MS = 250;
static constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000;

// --- Global Data Structures ---
static PendantStatePacket outgoing_pendant_data;
static LcncStatusPacket incoming_lcnc_data;

// --- Forward Declarations for Static Helper Functions ---
static void initialize_core_systems();
static void initialize_hmi_and_ui();
static void initialize_network_and_comms();
static void on_lcnc_data_received(const LcncStatusPacket &msg);
static void handle_core_tasks();
static void handle_pendant_data_sending();
static void handle_web_status_broadcast();

//================================================================================
// SETUP
//================================================================================
void setup()
{
    initialize_core_systems();
    initialize_hmi_and_ui();
    initialize_network_and_comms();
    web_interface_init();
    Serial.println("--- Setup Complete ---");
}

//================================================================================
// LOOP
//================================================================================
void loop()
{
    handle_core_tasks();
    handle_pendant_data_sending();
    handle_web_status_broadcast();
}

//================================================================================
// HELPER FUNCTION IMPLEMENTATIONS
//================================================================================

static void initialize_core_systems()
{
    Serial.begin(115200);
    Serial.println("\n--- ESP3 Pendant Booting ---");
    if (!LittleFS.begin(true))
    {
        Serial.println("FATAL: LittleFS mount failed. Halting.");
        while (true)
            yield();
    }
}

static void initialize_hmi_and_ui()
{
    load_pendant_configuration();
    hmi_pendant_init();
    lvgl_driver_init();
    ui_bridge_init();
    ui_popups_init();
    ui_tab_logic_init();
}

static void initialize_network_and_comms()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(Pinout::WIFI_SSID, Pinout::WIFI_PASSWORD);
    unsigned long start = millis();
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS)
    {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("\nWi-Fi Connected. IP: %s\n", WiFi.localIP().toString().c_str());
    }
    else
    {
        Serial.println("\nWARN: Wi-Fi failed to connect.");
    }

    communication_esp3_init();
    communication_esp3_register_receive_callback(on_lcnc_data_received);
}

static void on_lcnc_data_received(const LcncStatusPacket &msg)
{
    incoming_lcnc_data = msg;
    update_hmi_from_lcnc(msg);
    web_interface_broadcast_status(incoming_lcnc_data);
}

static void handle_core_tasks()
{
    lv_timer_handler();
    hmi_pendant_task();
    web_interface_loop();
}

static void handle_pendant_data_sending()
{
    static unsigned long last_pendant_send_ms = 0;
    if (hmi_pendant_data_has_changed() && (millis() - last_pendant_send_ms > PENDANT_SEND_INTERVAL_MS))
    {
        get_pendant_data(&outgoing_pendant_data);
        if (!communication_esp3_send(outgoing_pendant_data))
        {
            Serial.println("WARN: ESP-NOW send failed.");
        }
        last_pendant_send_ms = millis();
    }
}

static void handle_web_status_broadcast()
{
    static unsigned long last_status_broadcast_ms = 0;
    if (millis() - last_status_broadcast_ms > STATUS_BROADCAST_INTERVAL_MS)
    {
        uint32_t btns;
        int32_t hw;
        uint8_t axis, step;
        get_pendant_live_status(btns, hw, axis, step);
        web_interface_broadcast_live_pendant_status(btns, hw, axis, step);
        last_status_broadcast_ms = millis();
    }
}
// NOTE: The extra '}' at the end of the file has been removed.
