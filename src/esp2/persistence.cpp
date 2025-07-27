/**
 * @file persistence.cpp
 * @brief Implements the logic for saving, loading, and serializing
 * the dynamic web configuration using the Preferences and ArduinoJson libraries.
 *
 * This module is responsible for the permanent storage of user settings,
 * allowing the HMI's configuration to persist across reboots.
 *
 * Concepts are derived from the project document "Integriertes Dual-ESP32-Steuerungssystem".
 */

#include "persistence.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// --- GLOBAL OBJECTS ---

// Global instance of the configuration structure. This holds the live,
// in-memory configuration that the rest of the firmware uses.
WebConfig web_cfg;

// Instance of the Preferences library, used for accessing the ESP32's Non-Volatile Storage (NVS).
Preferences preferences;

// --- PRIVATE CONSTANTS ---

// A unique namespace within NVS to prevent key collisions with other libraries.
const char *PREFERENCES_NAMESPACE = "hmi-config";
// The key under which the entire configuration blob is stored.
const char *NVS_CONFIG_KEY = "web_config";

// --- FUNCTION IMPLEMENTATIONS ---

/**
 * @brief Loads the configuration from NVS into the global web_cfg object.
 * If no configuration is found in NVS (e.g., on first boot), the default
 * values initialized in the global web_cfg instance are used.
 */
void load_configuration()
{
    preferences.begin(PREFERENCES_NAMESPACE, false); // Open NVS in read/write mode

    // Check if a configuration key already exists.
    if (preferences.isKey(NVS_CONFIG_KEY))
    {
        // Key exists, so load the entire WebConfig struct as a binary blob from NVS.
        preferences.getBytes(NVS_CONFIG_KEY, &web_cfg, sizeof(web_cfg));
        if (DEBUG_ENABLED)
            Serial.println("Successfully loaded configuration from NVS.");
    }
    else
    {
        // No saved config found (e.g., first boot). The default values from the global
        // web_cfg instance (defined in its struct) will be used automatically.
        if (DEBUG_ENABLED)
            Serial.println("No configuration found in NVS. Using default values.");
    }
    preferences.end(); // Close NVS to free up resources.
}

/**
 * @brief Parses a JSON string received from the web UI and saves the new
 * configuration to NVS.
 * @param json_string The complete configuration as a JSON string.
 */
void save_configuration(const String &json_string)
{
    // Create a JSON document to parse the incoming JSON string from the web UI.
    // The size should be large enough to hold the entire config object.
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, json_string);

    if (error)
    {
        if (DEBUG_ENABLED)
        {
            Serial.print(F("Failed to parse saveConfig JSON: "));
            Serial.println(error.c_str());
        }
        return;
    }

    // --- Update the global web_cfg struct field-by-field from the parsed JSON ---

    // Update button settings
    JsonArray buttons = doc["buttons"].as<JsonArray>();
    for (int i = 0; i < buttons.size() && i < MAX_BUTTONS_DEFINED; ++i)
    {
        JsonObject btn = buttons[i];
        strlcpy(web_cfg.buttons[i].name, btn["name"] | "Button", sizeof(web_cfg.buttons[i].name));
        web_cfg.buttons[i].is_toggle = btn["is_toggle"] | false;
        web_cfg.buttons[i].radio_group_id = btn["radio_group_id"] | 0;
    }

    // Update LED settings
    JsonArray leds = doc["leds"].as<JsonArray>();
    for (int i = 0; i < leds.size() && i < MAX_LEDS; ++i)
    {
        JsonObject led = leds[i];
        strlcpy(web_cfg.leds[i].name, led["name"] | "LED", sizeof(web_cfg.leds[i].name));
        web_cfg.leds[i].binding_type = (LedBinding)(led["binding_type"] | 0);
        web_cfg.leds[i].bound_button_index = led["bound_button_index"] | -1;
        web_cfg.leds[i].lcnc_state_bit = led["lcnc_state_bit"] | -1;
    }

    // Update Joystick settings
    JsonArray joysticks = doc["joysticks"].as<JsonArray>();
    for (int i = 0; i < joysticks.size() && i < NUM_JOYSTICKS; ++i)
    {
        JsonArray axes = joysticks[i]["axes"].as<JsonArray>();
        for (int j = 0; j < axes.size() && j < NUM_JOYSTICK_AXES; ++j)
        {
            JsonObject axis = axes[j];
            web_cfg.joysticks[i][j].is_inverted = axis["is_inverted"] | false;
            web_cfg.joysticks[i][j].sensitivity = axis["sensitivity"] | 1.0f;
            web_cfg.joysticks[i][j].center_deadzone = axis["center_deadzone"] | 50;
        }
    }

    // --- Save the updated web_cfg struct as a binary blob to NVS ---
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putBytes(NVS_CONFIG_KEY, &web_cfg, sizeof(web_cfg));
    preferences.end();

    if (DEBUG_ENABLED)
        Serial.println("Successfully saved new configuration to NVS.");
}

/**
 * @brief Serializes the current, in-memory configuration into a JSON string.
 * This is used to send the current state to newly connected web clients
 * or for the generator script to fetch.
 * @return String The current configuration as a JSON object.
 */
String get_config_as_json()
{
    // Create a JSON document to build the JSON response string.
    StaticJsonDocument<4096> doc;

    // --- Build the JSON document from the global web_cfg struct ---

    // Build buttons array
    JsonArray buttons = doc.createNestedArray("buttons");
    for (int i = 0; i < MAX_BUTTONS_DEFINED; ++i)
    {
        JsonObject btn = buttons.createNestedObject();
        btn["name"] = web_cfg.buttons[i].name;
        btn["is_toggle"] = web_cfg.buttons[i].is_toggle;
        btn["radio_group_id"] = web_cfg.buttons[i].radio_group_id;
    }

    // Build LEDs array
    JsonArray leds = doc.createNestedArray("leds");
    for (int i = 0; i < MAX_LEDS; ++i)
    {
        JsonObject led = leds.createNestedObject();
        led["name"] = web_cfg.leds[i].name;
        led["binding_type"] = (int)web_cfg.leds[i].binding_type;
        led["bound_button_index"] = web_cfg.leds[i].bound_button_index;
        led["lcnc_state_bit"] = web_cfg.leds[i].lcnc_state_bit;
    }

    // Build joysticks array
    JsonArray joysticks = doc.createNestedArray("joysticks");
    for (int i = 0; i < NUM_JOYSTICKS; ++i)
    {
        JsonObject joy = joysticks.createNestedObject();
        joy["name"] = joystick_configs[i].name; // Get static name from config.h
        JsonArray axes = joy.createNestedArray("axes");
        for (int j = 0; j < NUM_JOYSTICK_AXES; ++j)
        {
            JsonObject axis = axes.createNestedObject();
            axis["is_inverted"] = web_cfg.joysticks[i][j].is_inverted;
            axis["sensitivity"] = web_cfg.joysticks[i][j].sensitivity;
            axis["center_deadzone"] = web_cfg.joysticks[i][j].center_deadzone;
        }
    }

    String json_output;
    serializeJson(doc, json_output);
    return json_output;
}