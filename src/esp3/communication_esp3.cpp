/**
 * @file communication_esp3.cpp
 * @brief Implements the ESP-NOW communication layer for the ESP32-S3 Pendant.
 */

#include "communication_esp3.h"
#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>

// --- Module-static (private) variables ---

// MAC address of the LinuxCNC bridge ESP32.
// IMPORTANT: Replace this with the actual MAC address of your receiver.
static uint8_t peer_mac_address[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// Pointer to the callback function provided by the main application.
static void (*on_receive_callback)(const LcncStatusPacket &msg) = nullptr;

// --- Internal ESP-NOW Callbacks ---

// This callback runs when data is received.
static void esp_now_receive_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    // Validate the message and ensure a callback is registered.
    if (len == sizeof(LcncStatusPacket) && on_receive_callback != nullptr)
    {
        // Safely cast the raw data to our struct and invoke the callback.
        on_receive_callback(*(LcncStatusPacket *)data);
    }
}

// This callback confirms if a message was sent successfully.
static void esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status != ESP_NOW_SEND_SUCCESS)
    {
        Serial.println("WARN: ESP-NOW send failed.");
    }
}

// --- Public API Functions ---

void communication_esp3_init()
{
    // ESP-NOW requires Wi-Fi in Station mode.
    WiFi.mode(WIFI_STA);
    // Setting the MAC address is optional but can be useful for debugging.
    // WiFi.macAddress(new_mac);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("FATAL: ESP-NOW initialization failed. Halting.");
        while (true)
        {
            delay(1000);
        }
    }

    esp_now_register_recv_cb(esp_now_receive_cb);
    esp_now_register_send_cb(esp_now_send_cb);

    // Register the receiver as a peer for reliable sending.
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, peer_mac_address, 6);
    peer_info.channel = 0; // 0 means use the current Wi-Fi channel.
    peer_info.encrypt = false;

    if (esp_now_add_peer(&peer_info) != ESP_OK)
    {
        Serial.println("WARN: Failed to add ESP-NOW peer.");
    }
}

void communication_esp3_register_receive_callback(void (*cb)(const LcncStatusPacket &msg))
{
    on_receive_callback = cb;
}

bool communication_esp3_send(const PendantStatePacket &msg)
{
    esp_err_t result = esp_now_send(peer_mac_address, (uint8_t *)&msg, sizeof(msg));
    return (result == ESP_OK);
}