/**
 * @file persistence.h
 * @brief Defines structures and functions for handling dynamic configuration.
 *
 * These settings are configured via the web interface and saved permanently
 * in the ESP32's Non-Volatile Storage (NVS). This allows user preferences
 * to persist across reboots.
 *
 * Concepts are derived from the project document "Integriertes Dual-ESP32-Steuerungssystem".
 */

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <Arduino.h>
#include "config_esp2.h" // Provides MAX_BUTTONS_DEFINED, MAX_LEDS, etc.

// --- DYNAMIC CONFIGURATION STRUCTURES ---

// Holds user-configurable settings for a single button.
struct ButtonDynamicConfig
{
    char name[32] = "Button";
    bool is_toggle = false;
    int radio_group_id = 0;
};

// Holds user-configurable settings for a single LED.
struct LedDynamicConfig
{
    char name[32] = "LED";
    LedBinding binding_type = LedBinding::UNBOUND;
    int bound_button_index = -1;
    int lcnc_state_bit = -1;
};

// Holds user-configurable settings for a single joystick axis.
struct JoystickAxisDynamicConfig
{
    bool is_inverted = false;
    float sensitivity = 1.0f;
    int center_deadzone = 50;
};

/**
 * @brief Master structure that holds the entire dynamic configuration.
 * An instance of this struct is what gets saved to and loaded from NVS
 * as a single binary blob for efficiency.
 */
struct WebConfig
{
    ButtonDynamicConfig buttons[MAX_BUTTONS_DEFINED];
    LedDynamicConfig leds[MAX_LEDS];
    JoystickAxisDynamicConfig joysticks[NUM_JOYSTICKS][NUM_JOYSTICK_AXES];
};

// --- FUNCTION PROTOTYPES ---

/**
 * @brief Loads the configuration from NVS into the global web_cfg object.
 * If no configuration is found in NVS (e.g., on first boot), the default
 * values initialized in the global web_cfg instance are used.
 */
void load_configuration();

/**
 * @brief Parses a JSON string received from the web UI and saves the new
 * configuration to NVS.
 * @param json_string The complete configuration as a JSON string.
 */
void save_configuration(const String &json_string);

/**
 * @brief Serializes the current, in-memory configuration into a JSON string.
 * This is used to send the current state to newly connected web clients
 * or for the generator script to fetch.
 * @return String The current configuration as a JSON object.
 */
String get_config_as_json();

#endif // PERSISTENCE_H