/**
 * @file persistence_esp3.cpp
 * @brief Implements loading/saving the web config using ESP32 Preferences and ArduinoJson.
 */

#include "persistence_esp3.h"
#include "ui.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// --- Custom JSON Converters for ArduinoJson v7 ---
// This is the new, required syntax for v7.
namespace ARDUINOJSON_NAMESPACE
{
    template <>
    struct Converter<BindingType>
    {
        static void toJson(BindingType src, JsonVariant dst) { dst.set(static_cast<uint8_t>(src)); }
        static BindingType fromJson(JsonVariantConst src) { return static_cast<BindingType>(src.as<int>()); }
        static bool checkJson(JsonVariantConst src) { return src.is<int>(); }
    };
    template <>
    struct Converter<LedBindingType>
    {
        static void toJson(LedBindingType src, JsonVariant dst) { dst.set(static_cast<uint8_t>(src)); }
        static LedBindingType fromJson(JsonVariantConst src) { return static_cast<LedBindingType>(src.as<int>()); }
        static bool checkJson(JsonVariantConst src) { return src.is<int>(); }
    };
}

// --- Global config object definition ---
extern PendantWebConfig pendant_web_cfg;
void run_factory_test() {}

// --- NVS (Preferences) constants ---
static constexpr char PENDANT_PREF_NS[] = "pendant";
static constexpr char PENDANT_PREF_KEY[] = "config";

// --- Internal Helper Function Prototypes ---
static void load_pendant_default_configuration();
static void serialize_config_to_json(JsonDocument &doc, const PendantWebConfig &cfg);
static void deserialize_config_from_json(PendantWebConfig &cfg, const JsonDocument &doc);

// --- Public API Implementation ---

void load_pendant_configuration()
{
    load_pendant_default_configuration();
    Preferences prefs;
    if (prefs.begin(PENDANT_PREF_NS, true))
    {
        String json_string = prefs.getString(PENDANT_PREF_KEY, "");
        prefs.end();
        if (!json_string.isEmpty())
        {
            JsonDocument doc; // Use dynamic document for v7
            if (deserializeJson(doc, json_string) == DeserializationError::Ok)
            {
                deserialize_config_from_json(pendant_web_cfg, doc);
                Serial.println("INFO: Loaded configuration from NVS.");
            }
            else
            {
                Serial.println("ERROR: Failed to parse config JSON from NVS.");
            }
        }
        else
        {
            Serial.println("INFO: No config found in NVS, using defaults.");
        }
    }
    else
    {
        Serial.println("WARN: Could not open NVS to load config.");
    }
    ui_bridge_apply_config(pendant_web_cfg);
}

void update_hmi_from_config()
{
    ui_bridge_apply_config(pendant_web_cfg);
}

void save_pendant_configuration(const String &json_string)
{
    Preferences prefs;
    if (prefs.begin(PENDANT_PREF_NS, false))
    {
        prefs.putString(PENDANT_PREF_KEY, json_string);
        prefs.end();
    }
    else
    {
        Serial.println("ERROR: Could not open NVS to save config.");
    }
}

String get_pendant_config_as_json()
{
    JsonDocument doc; // Use dynamic document for v7
    serialize_config_to_json(doc, pendant_web_cfg);
    String json_output;
    serializeJson(doc, json_output);
    return json_output;
}

void reset_pendant_to_defaults()
{
    load_pendant_default_configuration();
    String default_json = get_pendant_config_as_json();
    save_pendant_configuration(default_json);
    Serial.println("INFO: Config reset to defaults.");
    ui_bridge_apply_config(pendant_web_cfg);
}

// --- Internal Helper Function Implementations ---

static void load_pendant_default_configuration()
{
    pendant_web_cfg = {};
    pendant_web_cfg.num_dro_axes = 3;
    pendant_web_cfg.handwheel_enable_button = 0;
    pendant_web_cfg.axis_labels = {"X", "Y", "Z"};
    pendant_web_cfg.button_bindings = {{0, "Cycle Start", BindingType::BOUND_TO_LCNC, 101}};
    pendant_web_cfg.led_bindings = {{0, "Spindle On", LedBindingType::LED_BOUND_TO_MATRIX, 0, 0, 0}};
    pendant_web_cfg.macros = {{"M8 Coolant On", {"M8"}, 300}};
}

static void deserialize_config_from_json(PendantWebConfig &cfg, const JsonDocument &doc)
{
    cfg.num_dro_axes = doc["num_dro_axes"] | cfg.num_dro_axes;
    cfg.handwheel_enable_button = doc["handwheel_enable_button"] | cfg.handwheel_enable_button;

    if (doc["button_bindings"].is<JsonArray>())
    {
        JsonArrayConst button_array = doc["button_bindings"].as<JsonArrayConst>();
        cfg.button_bindings.clear();
        for (JsonObjectConst obj : button_array)
        {
            cfg.button_bindings.push_back({obj["button_index"], obj["action_name"] | "", obj["type"].as<BindingType>(), obj["lcnc_action_code"]});
        }
    }

    if (doc["led_bindings"].is<JsonArray>())
    {
        JsonArrayConst led_array = doc["led_bindings"].as<JsonArrayConst>();
        cfg.led_bindings.clear();
        for (JsonObjectConst obj : led_array)
        {
            cfg.led_bindings.push_back({obj["led_index"], obj["signal_name"] | "", obj["type"].as<LedBindingType>(), obj["matrix_index"], obj["bit_index"], obj["button_index"]});
        }
    }

    if (doc["macros"].is<JsonArray>())
    {
        JsonArrayConst macro_array = doc["macros"].as<JsonArrayConst>();
        cfg.macros.clear();
        for (JsonObjectConst obj : macro_array)
        {
            MacroEntry m;
            m.name = obj["name"] | "";
            m.action_code = obj["action_code"];
            if (obj["commands"].is<JsonArray>())
            {
                JsonArrayConst commands = obj["commands"].as<JsonArrayConst>();
                for (const char *cmd : commands)
                {
                    m.commands.push_back(cmd);
                }
            }
            cfg.macros.push_back(m);
        }
    }
}

static void serialize_config_to_json(JsonDocument &doc, const PendantWebConfig &cfg)
{
    doc["num_dro_axes"] = cfg.num_dro_axes;
    doc["handwheel_enable_button"] = cfg.handwheel_enable_button;

    JsonArray axisLabels = doc.createNestedArray("axis_labels");
    for (const auto &label : cfg.axis_labels)
    {
        axisLabels.add(label);
    }

    JsonArray btnArr = doc.createNestedArray("button_bindings");
    for (const auto &b : cfg.button_bindings)
    {
        JsonObject obj = btnArr.createNestedObject();
        obj["button_index"] = b.button_index;
        obj["type"] = b.type;
        obj["lcnc_action_code"] = b.lcnc_action_code;
        obj["action_name"] = b.action_name;
    }

    JsonArray ledArr = doc.createNestedArray("led_bindings");
    for (const auto &l : cfg.led_bindings)
    {
        JsonObject obj = ledArr.createNestedObject();
        obj["led_index"] = l.led_index;
        obj["type"] = l.type;
        obj["signal_name"] = l.signal_name;
        obj["matrix_index"] = l.matrix_index;
        obj["bit_index"] = l.bit_index;
        obj["button_index"] = l.button_index;
    }

    JsonArray macroArr = doc.createNestedArray("macros");
    for (const auto &m : cfg.macros)
    {
        JsonObject obj = macroArr.createNestedObject();
        obj["name"] = m.name;
        obj["action_code"] = m.action_code;
        JsonArray commands = obj.createNestedArray("commands");
        for (const auto &cmd : m.commands)
        {
            commands.add(cmd);
        }
    }
}
