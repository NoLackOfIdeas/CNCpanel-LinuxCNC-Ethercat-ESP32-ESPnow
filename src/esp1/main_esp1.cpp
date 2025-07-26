#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "config_esp1.h"
#include "shared_structures.h"
#include "EasyCAT.h" // Lokale Bibliothek
// #include "MyData.h" // WICHTIG: Binden Sie hier die vom Easy Configurator generierte Header-Datei ein
#include "SM_Test_myCustomSlave.h"
#include "ESP32Encoder.h"

// Globale Objekte und Variablen
EasyCAT EASYCAT;
ESP32Encoder encoders;
volatile uint32_t hall_pulse_count = 0;
unsigned long last_rpm_calc_time = 0;
uint32_t current_rpm = 0;
volatile bool probe_states = {false};

struct_message_to_esp1 incoming_hmi_data;
struct_message_to_esp2 outgoing_lcnc_data;

// --- ESP-NOW Callbacks ---
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (DEBUG_ENABLED)
    {
        Serial.print("\r\nLast Packet Send Status:\t");
        Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    memcpy(&incoming_hmi_data, incomingData, sizeof(incoming_hmi_data));
    if (DEBUG_ENABLED)
    {
        Serial.println("Data received from HMI.");
    }
}

// --- Interrupt Service Routines ---
void IRAM_ATTR hall_sensor_isr()
{
    hall_pulse_count++;
}

void IRAM_ATTR probe_isr()
{
    for (int i = 0; i < NUM_PROBES; i++)
    {
        probe_states[i] = digitalRead(PROBE_PINS[i]);
    }
}

void setup()
{
    if (DEBUG_ENABLED)
    {
        Serial.begin(115200);
        Serial.println("ESP1 (EtherCAT Bridge) starting...");
    }

    // WiFi für ESP-NOW initialisieren
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        if (DEBUG_ENABLED)
            Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // Peer für ESP2 hinzufügen
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, esp2_mac_address, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        if (DEBUG_ENABLED)
            Serial.println("Failed to add peer");
        return;
    }

    // EasyCAT initialisieren
    if (EASYCAT.Init() == true)
    {
        if (DEBUG_ENABLED)
            Serial.println("EasyCAT Init SUCCESS");
    }
    else
    {
        if (DEBUG_ENABLED)
            Serial.println("EasyCAT Init FAILED");
        while (1)
        {
            delay(100);
        }
    }

    // Peripherie initialisieren
    for (int i = 0; i < NUM_ENCODERS; i++)
    {
        if (ENCODER_A_PINS[i] != -1 && ENCODER_B_PINS[i] != -1)
        {
            ESP32Encoder::useInternalWeakPullResistors = ESP32Encoder::UP;
            encoders[i].attachHalfQuad(ENCODER_A_PINS[i], ENCODER_B_PINS[i]);
            encoders[i].setFilter(ENCODER_GLITCH_FILTER);
            encoders[i].clearCount();
        }
    }

    pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), hall_sensor_isr, FALLING);

    for (int i = 0; i < NUM_PROBES; i++)
    {
        pinMode(PROBE_PINS[i], INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PROBE_PINS[i]), probe_isr, CHANGE);
    }

    if (DEBUG_ENABLED)
        Serial.println("Setup complete.");
}

void loop()
{
    EASYCAT.MainTask();

    // --- DATEN VON ESP1-PERIPHERIE IN ETHERCAT-BUFFER SCHREIBEN ---
    // Beispielhaftes Mapping. Passen Sie dies an Ihre `MyData.h` an.
    /*
    for (int i = 0; i < NUM_ENCODERS; i++) {
        if (ENCODER_A_PINS[i]!= -1) {
            EASYCAT.BufferIn.Cust.encoder_values_esp1[i] = encoders[i].getCount();
        }
    }
    */

    if (millis() - last_rpm_calc_time >= TACHO_UPDATE_INTERVAL_MS)
    {
        noInterrupts();
        uint32_t pulses = hall_pulse_count;
        hall_pulse_count = 0;
        interrupts();

        current_rpm = (pulses * (60000 / TACHO_UPDATE_INTERVAL_MS)) / TACHO_MAGNETS_PER_REVOLUTION;
        if (current_rpm < TACHO_MIN_RPM_DISPLAY)
        {
            current_rpm = 0;
        }
        last_rpm_calc_time = millis();
        // EASYCAT.BufferIn.Cust.spindle_rpm = current_rpm;
    }

    uint8_t probe_bitmask = 0;
    for (int i = 0; i < NUM_PROBES; i++)
    {
        if (!probe_states[i])
        { // NPN Sensoren sind bei Detektion LOW
            probe_bitmask |= (1 << i);
        }
    }
    // EASYCAT.BufferIn.Cust.probe_states = probe_bitmask;

    // --- DATEN VON ESP2 (HMI) IN ETHERCAT-BUFFER SCHREIBEN ---
    // memcpy(&EASYCAT.BufferIn.Cust.button_states, &incoming_hmi_data.logical_button_states, sizeof(uint64_t));
    // for (int i = 0; i < MAX_ENCODERS_ESP2; i++) {
    //     EASYCAT.BufferIn.Cust.hmi_encoders[i] = incoming_hmi_data.encoder_values[i];
    // }
    // for (int i = 0; i < MAX_POTIS; i++) {
    //     EASYCAT.BufferIn.Cust.hmi_potis[i] = incoming_hmi_data.poti_values[i];
    // }

    // --- DATEN VON ETHERCAT (LINUXCNC) AN ESP2 (HMI) SENDEN ---
    // outgoing_lcnc_data.led_states_from_lcnc = EASYCAT.BufferOut.Cust.led_states_for_hmi;
    // outgoing_lcnc_data.linuxcnc_status = EASYCAT.BufferOut.Cust.linuxcnc_status_word;

    esp_now_send(esp2_mac_address, (uint8_t *)&outgoing_lcnc_data, sizeof(outgoing_lcnc_data));

    delay(1); // Kurze Pause, um den Watchdog nicht zu blockieren
}