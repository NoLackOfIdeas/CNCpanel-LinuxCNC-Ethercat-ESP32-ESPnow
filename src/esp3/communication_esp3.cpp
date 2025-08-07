#include "communication_esp3.h"
#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>

// callback pointer set by main
static void (*on_receive_cb)(const struct_message_to_hmi &msg) = nullptr;

// broadcast MAC for ESP-NOW
static uint8_t broadcast_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// low-level receive callback from ESP-NOW
static void espnow_recv_cb(
    const uint8_t *mac_addr,
    const uint8_t *incoming_data,
    int len)
{
    if (len != sizeof(struct_message_to_hmi) || on_receive_cb == nullptr)
    {
        return;
    }
    struct_message_to_hmi msg;
    memcpy(&msg, incoming_data, sizeof(msg));
    on_receive_cb(msg);
}

void communication_esp3_init()
{
    // Put Wi-Fi in STA mode (ESP-NOW requires it)
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ERROR: ESP-NOW init failed");
        return;
    }

    // Register receive callback
    esp_now_register_recv_cb(espnow_recv_cb);

    // Add a broadcast peer so we can send to any listener
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcast_addr, 6);
    peerInfo.channel = 0; // use current Wi-Fi channel
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("WARN: failed to add ESP-NOW broadcast peer");
    }
}

void communication_esp3_register_receive_callback(
    void (*cb)(const struct_message_to_hmi &msg))
{
    on_receive_cb = cb;
}

void communication_esp3_loop()
{
    // ESP-NOW needs no polling; placeholder for timeouts/retries if needed
}

bool communication_esp3_send(const struct_message_from_esp3 &msg)
{
    // Fire-and-forget broadcast
    esp_err_t res = esp_now_send(
        broadcast_addr,
        (uint8_t *)&msg,
        sizeof(msg));

    bool ok = (res == ESP_OK);
    if (!ok)
    {
        Serial.printf("ERROR: ESP-NOW send failed (%d), retrying...\n", res);
        // Single retry
        res = esp_now_send(broadcast_addr, (uint8_t *)&msg, sizeof(msg));
        ok = (res == ESP_OK);
        Serial.printf("ESP-NOW send retry %s\n", ok ? "succeeded" : "failed");
    }
    return ok;
}
