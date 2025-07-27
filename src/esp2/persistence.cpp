/**
 * @file persistence.cpp
 * @brief Implements the logic for saving, loading, and serializing
 * the dynamic web configuration using the Preferences and ArduinoJson libraries.
 */

#include "persistence.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// --- GLOBAL OBJECTS ---
WebConfig web_cfg;
Preferences preferences;

// --- PRIVATE CONSTANTS ---
const char *PREFERENCES_NAMESPACE = "hmi-config";
const char *NVS_CONFIG_KEY = "web_config";

// --- FUNCTION IMPLEMENTATIONS ---

/**
 * @brief Loads the configuration from NVS. (No changes needed here)
 */
void load_configuration()
{
    preferences.begin(PREFERENCES_NAMESPACE, false);
    if (preferences.isKey(NVS_CONFIG_KEY))
    {
        preferences.getBytes(NVS_CONFIG_KEY, &web_cfg, sizeof(web_cfg));
        if (DEBUG_ENABLED)
            Serial.println("Successfully loaded configuration from NVS.");
    }
    else
    {
        if (DEBUG_ENABLED)
            Serial.println("No configuration found in NVS. Using default values.");
    }
    preferences.end();
}

/**
 * @brief Saves the configuration received from the web UI to NVS.
 */
void save_configuration(const String &json_string)
{
    StaticJsonDocument<4096> doc; // Increased size for new bindings array
    DeserializationError error = deserializeJson(doc, json_string);

    if (error)
    {
        if (DEBUG_ENABLED)
            Serial.printf("deserializeJson() failed: %s\n", error.c_str());
        return;
    }

    // --- Update button settings (no changes here) ---
    JsonArray buttons = doc["buttons"].as<JsonArray>();
    for (int i = 0; i < buttons.size() && i < MAX_BUTTONS_DEFINED; ++i)
    {
        // ...
    }

    // --- Update LED settings (no changes here) ---
    JsonArray leds = doc["leds"].as<JsonArray>();
    for (int i = 0; i < leds.size() && i < MAX_LEDS; ++i)
    {
        // ...
    }

    // --- Update Joystick settings (no changes here) ---
    JsonArray joysticks = doc["joysticks"].as<JsonArray>();
    for (int i = 0; i < joysticks.size() && i < NUM_JOYSTICKS; ++i)
    {
        // ...
    }

    // --- NEW: Update Action Binding settings ---
    JsonArray bindings = doc["bindings"].as<JsonArray>();
    for (int i = 0; i < bindings.size() && i < MAX_ACTION_BINDINGS; ++i)
    {
        JsonObject binding = bindings[i];
        web_cfg.bindings[i].is_active = binding["is_active"] | false;
        web_cfg.bindings[i].trigger = (TriggerType)(binding["trigger"] | 0);
        web_cfg.bindings[i].action = (ActionType)(binding["action"] | 0);
    }

    // --- Save the updated web_cfg struct to NVS (no changes here) ---
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putBytes(NVS_CONFIG_KEY, &web_cfg, sizeof(web_cfg));
    preferences.end();

    if (DEBUG_ENABLED)
        Serial.println("Successfully saved new configuration to NVS.");
}

/**
 * @brief Serializes the current configuration to a JSON string.
 */
String get_config_as_json()
{
    StaticJsonDocument<4096> doc; // Increased size for new bindings array

    // --- Build buttons array (no changes here) ---
    JsonArray buttons = doc.createNestedArray("buttons");
    for (int i = 0; i < MAX_BUTTONS_DEFINED; ++i)
    {
        // ...
    }

    // --- Build LEDs array (no changes here) ---
    JsonArray leds = doc.createNestedArray("leds");
    for (int i = 0; i < MAX_LEDS; ++i)
    {
        // ...
    }

    // --- Build joysticks array (no changes here) ---
    JsonArray joysticks = doc.createNestedArray("joysticks");
    for (int i = 0; i < NUM_JOYSTICKS; ++i)
    {
        // ...
    }

    // --- NEW: Build Action Bindings array ---
    JsonArray bindings = doc.createNestedArray("bindings");
    for (int i = 0; i < MAX_ACTION_BINDINGS; ++i)
    {
        JsonObject binding = bindings.createNestedObject();
        binding["is_active"] = web_cfg.bindings[i].is_active;
        binding["trigger"] = (int)web_cfg.bindings[i].trigger;
        binding["action"] = (int)web_cfg.bindings[i].action;
    }

    String json_output;
    serializeJson(doc, json_output);
    return json_output;
}