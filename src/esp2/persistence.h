/**
 * @file persistence.h
 * @brief Defines structures and functions for handling dynamic configuration.
 *
 * These settings are configured via the web interface and saved permanently
 * in the ESP32's Non-Volatile Storage (NVS). This allows user preferences
 * to persist across reboots.
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

// --- NEW: Action Binding Definitions ---

/**
 * @brief Defines all possible machine states that can be used as a trigger.
 * The integer values MUST correspond to the bit positions in the status words
 * from the MyData.h file (machine_status and spindle_coolant_status).
 */
enum TriggerType
{
    // --- Triggers from machine_status word (bits 0-15) ---
    MACHINE_IS_ON = 0,
    IN_AUTO_MODE = 1,
    IN_MDI_MODE = 2,
    IN_JOG_MODE = 3,
    PROGRAM_IS_RUNNING = 4,
    PROGRAM_IS_PAUSED = 5,
    ON_HOME_POSITION = 6,
    // --- Triggers from spindle_coolant_status word (bits 16-31) ---
    // We add 16 to distinguish them from the first status word's bits.
    SPINDLE_IS_ON = 16,
    SPINDLE_AT_SPEED = 17,
    MIST_IS_ON = 18,
    FLOOD_IS_ON = 19,
};

/**
 * @brief Defines all possible actions the HMI can perform based on a trigger.
 */
enum ActionType
{
    NO_ACTION,
    ENABLE_JOYSTICK_1,
    DISABLE_JOYSTICK_1,
    // Add other actions here, e.g., for enabling/disabling button groups
};

/**
 * @brief Represents a single user-defined rule (a binding).
 * This links a machine state (Trigger) to an HMI behavior (Action).
 */
struct ActionBinding
{
    TriggerType trigger = MACHINE_IS_ON;
    ActionType action = NO_ACTION;
    bool is_active = false; // To enable/disable the rule in the UI
};

#define MAX_ACTION_BINDINGS 16 // Allow for up to 16 user-defined rules

/**
 * @brief Master structure that holds the entire dynamic configuration.
 * An instance of this struct is what gets saved to and loaded from NVS.
 */
struct WebConfig
{
    ButtonDynamicConfig buttons[MAX_BUTTONS_DEFINED];
    LedDynamicConfig leds[MAX_LEDS];
    JoystickAxisDynamicConfig joysticks[NUM_JOYSTICKS][NUM_JOYSTICK_AXES];
    ActionBinding bindings[MAX_ACTION_BINDINGS]; // NEW: Added the array of bindings
};

// --- FUNCTION PROTOTYPES ---

/**
 * @brief Loads the configuration from NVS into the global web_cfg object.
 */
void load_configuration();

/**
 * @brief Parses a JSON string and saves the new configuration to NVS.
 * @param json_string The complete configuration as a JSON string.
 */
void save_configuration(const String &json_string);

/**
 * @brief Serializes the current configuration into a JSON string.
 * @return String The current configuration as a JSON object.
 */
String get_config_as_json();

#endif // PERSISTENCE_H