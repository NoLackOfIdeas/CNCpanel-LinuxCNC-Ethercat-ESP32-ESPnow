/**
 * @file main_esp1.cpp
 * @brief Firmware for ESP1, the Real-Time Bridge (Fully Implemented).
 *
 * This code is responsible for:
 * 1. Communicating with LinuxCNC via EtherCAT using the EasyCAT library.
 * 2. Reading high-speed local sensors (encoders, probes) and calculating values (RPM).
 * 3. Bridging data to and from the ESP2 HMI controller via the ESP-NOW protocol.
 *
 * The data structures for EtherCAT are defined in MyData.h, which is
 * manually created by the user from the EasyCAT Configurator tool.
 *
 * Concepts are derived from the project document "Integriertes Dual-ESP32-Steuerungssystem".
 */

// --- DEFINES & INCLUDES ---
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "config_esp1.h"
#include "shared_structures.h"
#include "ESP32Encoder.h"

// --- CRITICAL SECTION FOR EASYCAT CUSTOMIZATION ---

// STEP 1: Define the macro that enables custom PDOs and "#define CUSTOM"
// This MUST come before including EasyCAT.h.
#define CUSTOM

// STEP 2: Include your manually created data structure.
#include "MyData.h"

// STEP 3: Now, include the EasyCAT library. It will see the define and
// use the structures from MyData.h instead of its internal defaults.
#include "EasyCAT.h"

// --- GLOBAL OBJECTS AND STATE VARIABLES ---
EasyCAT EASYCAT;                                  // The EasyCAT library instance
ESP32Encoder encoders[NUM_ENCODERS];              // Encoder objects
volatile uint32_t hall_pulse_count = 0;           // Pulse counter for RPM, modified by an ISR
volatile bool probe_states[NUM_PROBES] = {false}; // Array to hold probe states, modified by ISRs
unsigned long last_rpm_calc_time = 0;             // Timer for non-blocking RPM calculation
struct_message_to_esp1 incoming_hmi_data;         // Buffer for data received from ESP2
struct_message_to_esp2 outgoing_lcnc_data;        // Buffer for data to be sent to ESP2

// --- ESP-NOW CALLBACKS ---

/**
 * @brief Callback function executed when data is received from ESP2.
 * Copies the incoming HMI data into a local buffer for processing in the main loop.
 */
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    memcpy(&incoming_hmi_data, incomingData, sizeof(incoming_hmi_data));
}

/**
 * @brief Callback function executed after an ESP-NOW packet has been sent.
 * Can be used for debugging send status.
 */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (DEBUG_ENABLED && status != ESP_NOW_SEND_SUCCESS)
    {
        Serial.println("ESP-NOW send failed.");
    }
}

// --- INTERRUPT SERVICE ROUTINES (ISRs) ---
// These functions are designed to be extremely fast to avoid delaying the main program.

void IRAM_ATTR hall_sensor_isr() { hall_pulse_count++; }

/**
 * @brief Generic ISR for all probes.
 * This single function handles interrupts from any probe pin, making the code
 * flexible and scalable. It uses the 'arg' parameter to identify which probe triggered it.
 * @param arg A void pointer to the index of the probe (0, 1, 2, etc.).
 */
void IRAM_ATTR probe_isr_handler(void *arg)
{
    // Cast the void pointer argument back to an integer index.
    int probe_index = (int)arg;

    // Update the state for the specific probe that triggered the interrupt.
    if (probe_index < NUM_PROBES)
    {
        probe_states[probe_index] = digitalRead(PROBE_PINS[probe_index]);
    }
}

// --- MAIN SETUP AND LOOP ---

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting ESP1 - EtherCAT Bridge...");

    // -- 1. Initialize networking for ESP-NOW --
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_register_recv_cb(OnDataRecv);
    esp_now_register_send_cb(OnDataSent);

    // -- 2. Register ESP2 as an ESP-NOW peer --
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, esp2_mac_address, 6);
    esp_now_add_peer(&peerInfo);

    // -- 3. Initialize the EtherCAT slave controller --
    if (!EASYCAT.Init())
    {
        Serial.println("FATAL: EasyCAT Init FAILED. Halting.");
        while (1)
        {
            delay(100);
        } // Halt execution
    }
    Serial.println("EasyCAT Init SUCCESS.");

    // -- 4. Initialize local peripherals (encoders, sensors) --
    // Use the library's specific enum 'puType::up' to avoid name collisions with Arduino's PULLUP macro
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    for (int i = 0; i < NUM_ENCODERS; i++)
    {
        encoders[i].attachFullQuad(ENCODER_A_PINS[i], ENCODER_B_PINS[i]);
        encoders[i].clearCount();
    }

    pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), hall_sensor_isr, FALLING);

    // -- 5. Flexibly setup all configured probes --
    for (int i = 0; i < NUM_PROBES; i++)
    {
        pinMode(PROBE_PINS[i], INPUT_PULLUP);
    }

    // Flexibly attach an interrupt for each configured probe using a loop.
    for (int i = 0; i < NUM_PROBES; i++)
    {
        // attachInterruptArg is an ESP32-specific function that allows passing an argument (the index 'i') to the ISR.
        // The 4th argument 'CHANGE' specifies the interrupt mode.
        attachInterruptArg(digitalPinToInterrupt(PROBE_PINS[i]), probe_isr_handler, (void *)i, CHANGE);
    }

    Serial.println("ESP1 Setup Complete. Running...");
}

void loop()
{
    // 1. Run the main state machine for the EtherCAT library. This is critical and must be called frequently.
    EASYCAT.MainTask();

    // 2. Read local high-speed sensors and write their values into the EtherCAT IN buffer.
    for (int i = 0; i < NUM_ENCODERS; i++)
    {
        EASYCAT.BufferIn.Cust.enc_pos[i] = encoders[i].getCount();
    }

    // Non-blocking RPM calculation
    if (millis() - last_rpm_calc_time >= TACHO_UPDATE_INTERVAL_MS)
    {
        noInterrupts();
        uint32_t pulses = hall_pulse_count;
        hall_pulse_count = 0;
        interrupts();
        uint32_t rpm = (pulses * (60000 / TACHO_UPDATE_INTERVAL_MS)) / TACHO_MAGNETS_PER_REVOLUTION;
        EASYCAT.BufferIn.Cust.spindle_rpm = rpm;
        last_rpm_calc_time = millis();
    }

    // Pack probe states into a single byte (bitmask)
    uint8_t probe_bitmask = 0;
    for (int i = 0; i < NUM_PROBES; i++)
    {
        if (probe_states[i])
        {
            bitSet(probe_bitmask, i);
        }
    }
    EASYCAT.BufferIn.Cust.probe_states = probe_bitmask;

    // 3. Bridge data from ESP2 (HMI) into the EtherCAT IN buffer to make it available to LinuxCNC.
    memcpy(EASYCAT.BufferIn.Cust.button_matrix, incoming_hmi_data.button_matrix_states, sizeof(EASYCAT.BufferIn.Cust.button_matrix));
    memcpy(EASYCAT.BufferIn.Cust.joystick_axes, incoming_hmi_data.joystick_values, sizeof(EASYCAT.BufferIn.Cust.joystick_axes));

    // 4. Bridge data from the EtherCAT OUT buffer (from LinuxCNC) to ESP2 to update the HMI.
    memcpy(outgoing_lcnc_data.led_matrix_states, EASYCAT.BufferOut.Cust.led_matrix, sizeof(outgoing_lcnc_data.led_matrix_states));
    outgoing_lcnc_data.linuxcnc_status = EASYCAT.BufferOut.Cust.lcnc_status_word;
    esp_now_send(esp2_mac_address, (uint8_t *)&outgoing_lcnc_data, sizeof(outgoing_lcnc_data));
}