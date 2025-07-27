/**
 * @file shared_structures.h
 * @brief Defines data structures shared between ESP1 and ESP2.
 * This file is crucial for ensuring data consistency for the ESP-NOW communication link.
 */

#ifndef SHARED_STRUCTURES_H
#define SHARED_STRUCTURES_H
#include <stdint.h>

// --- HARDWARE ADDRESSES ---
// IMPORTANT: You must enter the MAC addresses of your specific ESP32 boards here.
static uint8_t esp1_mac_address[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // MAC address of ESP1 (Bridge)
static uint8_t esp2_mac_address[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0xEF}; // MAC address of ESP2 (HMI)

// --- CONFIGURATION CONSTANTS ---
#define MAX_JOYSTICKS 1

// --- ESP-NOW DATA STRUCTURES ---

/**
 * @brief Data packet sent from ESP2 (HMI) to ESP1 (Bridge).
 */
typedef struct struct_message_to_esp1
{
    uint8_t button_matrix_states[8];
    int16_t joystick_values[MAX_JOYSTICKS * 3];
    bool data_changed;
} struct_message_to_esp1;

/**
 * @brief Data packet sent from ESP1 (Bridge) to ESP2 (HMI).
 */
typedef struct struct_message_to_esp2
{
    uint8_t led_matrix_states[8];
    uint32_t linuxcnc_status;
    // --- NEW: Added fields to match the extended MyData.h ---
    uint16_t machine_status;         // Bitmask for machine states (is_on, mode, etc.)
    uint16_t spindle_coolant_status; // Bitmask for spindle/coolant states
} struct_message_to_esp2;

#endif // SHARED_STRUCTURES_H