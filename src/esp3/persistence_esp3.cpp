/**
 * @file persistence_esp3.cpp
 * @brief Implements loading/saving JSON-based web config using ESP32 Preferences + ArduinoJson.
 */

#include "persistence_esp3.h"
#include "config_esp3.h" // for load_pendant_default_configuration()
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// Global in-RAM config (declared extern in persistence_esp3.h)
extern PendantWebConfig pendant_web_cfg;

// NVS namespace & key
static constexpr char PENDANT_PREF_NS[] = "pendant";
static constexpr char PENDANT_PREF_KEY[] = "config";

/**
 * Load compile-time defaults into pendant_web_cfg.
 * Defined in config_esp3.cpp.
 */
extern void load_pendant_default_configuration();

/**
 * Load from NVS, parse JSON, fill pendant_web_cfg.
 * Starts by loading compile-time defaults, then overrides.
 */
void load_pendant_configuration()
{
    // 1) Populate RAM with factory defaults
    load_pendant_default_configuration();

    // 2) Open NVS (read-only)
    Preferences prefs;
    if (!prefs.begin(PENDANT_PREF_NS, true))
    {
        Serial.println("WARN: NVS begin failed in load_pendant_configuration()");
        return;
    }

    // 3) Fetch stored JSON
    String json = prefs.getString(PENDANT_PREF_KEY, "");
    prefs.end();

    if (json.isEmpty())
    {
        Serial.println("INFO: No stored config; using defaults");
        return;
    }

    // 4) Parse and apply overrides
    StaticJsonDocument<2048> doc;
    auto err = deserializeJson(doc, json);
    if (err)
    {
        Serial.printf("ERROR: JSON parse failed: %s\n", err.c_str());
        return;
    }

    // Override scalar fields if present
    if (doc.containsKey("num_dro_axes"))
        pendant_web_cfg.num_dro_axes = doc["num_dro_axes"].as<uint8_t>();

    if (doc.containsKey("handwheel_enable_button"))
        pendant_web_cfg.handwheel_enable_button = doc["handwheel_enable_button"].as<uint8_t>();

    // Override button_bindings
    pendant_web_cfg.button_bindings.clear();
    if (auto arr = doc["button_bindings"].as<JsonArray>())
    {
        for (JsonObject obj : arr)
        {
            ButtonBinding b{};
            b.type = static_cast<BindingType>(obj["type"].as<uint8_t>());
            b.button_index = obj["button_index"].as<uint8_t>();
            b.lcnc_action_code = obj["lcnc_action_code"].as<uint16_t>();
            b.action_name = obj["action_name"].as<const char *>();
            pendant_web_cfg.button_bindings.push_back(b);
        }
    }

    // Override led_bindings
    pendant_web_cfg.led_bindings.clear();
    if (auto arr = doc["led_bindings"].as<JsonArray>())
    {
        for (JsonObject obj : arr)
        {
            LedBinding l{};
            l.type = static_cast<LedBindingType>(obj["type"].as<uint8_t>());
            l.led_index = obj["led_index"].as<uint8_t>();
            l.signal_name = obj["signal_name"].as<const char *>();
            l.matrix_index = obj["matrix_index"].as<uint8_t>();
            l.bit_index = obj["bit_index"].as<uint8_t>();
            l.button_index = obj["button_index"].as<uint8_t>();
            pendant_web_cfg.led_bindings.push_back(l);
        }
    }

    // Override macros
    pendant_web_cfg.macros.clear();
    if (auto arr = doc["macros"].as<JsonArray>())
    {
        for (JsonObject obj : arr)
        {
            MacroEntry m{};
            m.name = obj["name"] | "";
            m.action_code = obj["action_code"].as<uint16_t>();

            if (auto cmdArr = obj["commands"].as<JsonArray>())
            {
                for (const char *cmd : cmdArr)
                    m.commands.push_back(cmd);
            }

            pendant_web_cfg.macros.push_back(m);
        }
    }
}

/**
 * Save a JSON string into NVS under key "config".
 */
void save_pendant_configuration(const String &json_string)
{
    Preferences prefs;
    if (!prefs.begin(PENDANT_PREF_NS, false))
    {
        Serial.println("ERROR: NVS begin failed in save_pendant_configuration()");
        return;
    }
    prefs.putString(PENDANT_PREF_KEY, json_string);
    prefs.end();
}

/**
 * Serialize pendant_web_cfg into a JSON string for the web UI.
 */
String get_pendant_config_as_json()
{
    StaticJsonDocument<2048> doc;
    doc["num_dro_axes"] = pendant_web_cfg.num_dro_axes;
    doc["handwheel_enable_button"] = pendant_web_cfg.handwheel_enable_button;

    auto btnArr = doc.createNestedArray("button_bindings");
    for (auto &b : pendant_web_cfg.button_bindings)
    {
        JsonObject obj = btnArr.createNestedObject();
        obj["type"] = static_cast<uint8_t>(b.type);
        obj["button_index"] = b.button_index;
        obj["lcnc_action_code"] = b.lcnc_action_code;
        obj["action_name"] = b.action_name;
    }

    auto ledArr = doc.createNestedArray("led_bindings");
    for (auto &l : pendant_web_cfg.led_bindings)
    {
        JsonObject obj = ledArr.createNestedObject();
        obj["type"] = static_cast<uint8_t>(l.type);
        obj["led_index"] = l.led_index;
        obj["signal_name"] = l.signal_name;
        obj["matrix_index"] = l.matrix_index;
        obj["bit_index"] = l.bit_index;
        obj["button_index"] = l.button_index;
    }

    auto macroArr = doc.createNestedArray("macros");
    for (auto &m : pendant_web_cfg.macros)
    {
        JsonObject obj = macroArr.createNestedObject();
        obj["name"] = m.name;
        obj["action_code"] = m.action_code;

        auto cmdArr = obj.createNestedArray("commands");
        for (auto &cmd : m.commands)
            cmdArr.add(cmd);
    }

    String out;
    serializeJson(doc, out);
    return out;
}

/**
 * Reset in-RAM config to defaults and immediately persist them.
 */
void reset_pendant_to_defaults()
{
    load_pendant_default_configuration();
    String defaultJson = get_pendant_config_as_json();
    save_pendant_configuration(defaultJson);
    Serial.println("INFO: Pendant configuration reset to defaults.");
}

/**
 * Stub for factory test routine.
 */
void run_factory_test()
{
    Serial.println(">>> Starting factory test routine...");
    // TODO: exercise LEDs, buttons, sensors…
}
