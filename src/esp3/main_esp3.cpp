/**
 * @file main_esp3.cpp
 * @brief Main firmware for the ESP32-S3 Handheld Pendant.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <lvgl.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// --- Core Application Headers ---
#include "config_esp3.h"
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

// --- FreeRTOS Queue for Encoder Deltas ---
QueueHandle_t encoderDeltaQueue = nullptr;

// --- Forward Declarations ---
static void initialize_core_systems();
static void initialize_hmi_and_ui();
static void initialize_network_and_comms();
static void on_lcnc_data_received(const LcncStatusPacket &msg);
static void handle_core_tasks();
static void handle_pendant_data_sending();
static void handle_web_status_broadcast();

// This is our new “robust” loop task:
static void loopTask(void *pvParameters)
{
    int32_t diff;

    // Wait for LVGL to be ready
    while (!lvgl_initialized)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    Serial.println("DEBUG: loopTask starting - LVGL ready");

    while (true)
    {
        if (xQueueReceive(encoderDeltaQueue, &diff, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            ESP_LOGI("HANDWHEEL", "Encoder delta = %ld", diff);
        }

        lv_timer_handler(); // Now safe to call
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

//================================================================================
// SETUP
//================================================================================

void setup()
{
    initialize_core_systems();

    // Add PSRAM check right here:
    Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

    initialize_hmi_and_ui();
    initialize_network_and_comms();
    web_interface_init();

    // Create our encoder-delta queue
    encoderDeltaQueue = xQueueCreate(
        /* length */ 16,
        /* item size */ sizeof(int32_t));

    // Launch the loopTask on core 1 with priority 1
    xTaskCreatePinnedToCore(
        loopTask,
        "loopTask",
        /* stack depth */ 4 * 1024,
        /* parameters */ nullptr,
        /* priority */ 1,
        /* handle */ nullptr,
        /* core */ 1);

    Serial.println("--- Setup Complete ---");
}

//================================================================================
// LOOP
//================================================================================

void loop()
{
    // Nothing here—everything is now driven by FreeRTOS tasks.
    vTaskDelay(pdMS_TO_TICKS(1000));
}

//================================================================================
// HELPER FUNCTIONS
//================================================================================

static void initialize_core_systems()
{
    // 0) NVS init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK)
    {
        Serial.printf("FATAL: NVS init failed (%d)\n", ret);
        while (true)
        {
            yield();
        }
    }

    Serial.begin(115200);
    Serial.println("\n--- ESP3 Pendant Booting ---");

    // 1) LittleFS
    if (!LittleFS.begin(true))
    {
        Serial.println("FATAL: LittleFS mount failed");
        while (true)
        {
            yield();
        }
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
    Serial.println("DEBUG: Starting WiFi setup...");
    WiFi.mode(WIFI_STA);
    Serial.println("DEBUG: WiFi mode set to STA");

    WiFi.begin(Pinout::WIFI_SSID, Pinout::WIFI_PASSWORD);
    Serial.println("DEBUG: WiFi.begin() called");

    unsigned long start = millis();
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < WIFI_CONNECT_TIMEOUT_MS)
    {
        Serial.println("DEBUG: In WiFi delay loop"); // ← Add this
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("\nWi-Fi Connected. IP: %s\n",
                      WiFi.localIP().toString().c_str());
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
    // No longer called every cycle—it lives in loopTask now
}

static void handle_pendant_data_sending()
{
    static unsigned long last = 0;
    if (hmi_pendant_data_has_changed() &&
        (millis() - last > PENDANT_SEND_INTERVAL_MS))
    {
        get_pendant_data(&outgoing_pendant_data);
        if (!communication_esp3_send(outgoing_pendant_data))
        {
            Serial.println("WARN: ESP-NOW send failed.");
        }
        last = millis();
    }
}

static void handle_web_status_broadcast()
{
    static unsigned long last = 0;
    if (millis() - last > STATUS_BROADCAST_INTERVAL_MS)
    {
        uint32_t btns;
        int32_t hw;
        uint8_t axis, step;
        get_pendant_live_status(btns, hw, axis, step);
        web_interface_broadcast_live_pendant_status(btns, hw, axis, step);
        last = millis();
    }
}
