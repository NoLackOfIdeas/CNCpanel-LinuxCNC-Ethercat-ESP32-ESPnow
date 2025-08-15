/**
 * @file persistence_esp3.cpp
 * @brief Implements loading/saving the web config + handwheel settings
 *        using ESP32 Preferences and ArduinoJson v7.
 */

#include "persistence_esp3.h"
#include "ui.h"
#include <Preferences.h>
#include <ArduinoJson.h>

//--- Encoder config NVS keys + globals ---
static constexpr char ENC_NS[] = "encoder";
static constexpr char KEY_INV[] = "invert";
static constexpr char KEY_DZ[] = "dz";

bool encoder_inverted = false;
int16_t encoder_deadzone = 0;

//--- Safe Preferences wrapper --------------------------------------------------
bool safeBegin(Preferences &p, const char *namespaceName, bool readOnly = false)
{
    if (!p.begin(namespaceName, readOnly))
    {
        Serial.printf("⚠️ Preferences.begin failed for namespace '%s'. Attempting fallback...\n", namespaceName);
        if (readOnly)
        {
            if (!p.begin(namespaceName, false))
            {
                Serial.printf("❌ Fallback failed: Unable to open or create namespace '%s'.\n", namespaceName);
                return false;
            }
            else
            {
                Serial.printf("✅ Fallback succeeded: Namespace '%s' created in read-write mode.\n", namespaceName);
                return true;
            }
        }
        return false;
    }

    Serial.printf("✅ Preferences.begin succeeded for namespace '%s'.\n", namespaceName);
    return true;
}

//--- Load/Save encoder settings -----------------------------------------------
void load_encoder_config()
{
    Preferences p;
    if (!safeBegin(p, ENC_NS, true))
    {
        Serial.println("WARN: cannot open NVS for encoder config");
        return;
    }
    encoder_inverted = p.getBool(KEY_INV, false);
    encoder_deadzone = p.getInt(KEY_DZ, 0);
    p.end();
}

void save_encoder_config()
{
    Preferences p;
    if (!safeBegin(p, ENC_NS, false))
    {
        Serial.println("ERROR: cannot open NVS to save encoder config");
        return;
    }
    p.putBool(KEY_INV, encoder_inverted);
    p.putInt(KEY_DZ, encoder_deadzone);
    p.end();
}

//--- JSON converters for your binding enums ----------------------------------
namespace ARDUINOJSON_NAMESPACE
{
    template <>
    struct Converter<BindingType>
    {
        static void toJson(BindingType v, JsonVariant dst) { dst.set((uint8_t)v); }
        static BindingType fromJson(JsonVariantConst src) { return (BindingType)src.as<int>(); }
        static bool checkJson(JsonVariantConst src) { return src.is<int>(); }
    };

    template <>
    struct Converter<LedBindingType>
    {
        static void toJson(LedBindingType v, JsonVariant dst) { dst.set((uint8_t)v); }
        static LedBindingType fromJson(JsonVariantConst src) { return (LedBindingType)src.as<int>(); }
        static bool checkJson(JsonVariantConst src) { return src.is<int>(); }
    };
}

//--- Global web config --------------------------------------------------------
PendantWebConfig pendant_web_cfg;

//--- NVS constants for web config ---------------------------------------------
static constexpr char PENDANT_PREF_NS[] = "pendant";
static constexpr char PENDANT_PREF_KEY[] = "config";

//--- Forward decls for JSON routines ------------------------------------------
static void load_pendant_default_configuration();
static void deserialize_config_from_json(PendantWebConfig &cfg, const JsonDocument &doc);
static void serialize_config_to_json(JsonDocument &doc, const PendantWebConfig &cfg);

//================================================================================
// PUBLIC API
//================================================================================

void load_pendant_configuration()
{
    // 1) Load defaults into RAM
    load_pendant_default_configuration();

    // 2) Ensure NVS has a config key, initialize if missing, then read it
    Preferences p;
    bool canRead = false;

    // Try opening in read-write mode to potentially initialize defaults
    if (safeBegin(p, PENDANT_PREF_NS, /*readOnly=*/false))
    {
        if (!p.isKey(PENDANT_PREF_KEY))
        {
            String defaultJson = get_pendant_config_as_json();
            p.putString(PENDANT_PREF_KEY, defaultJson);
            Serial.println("INFO: Initialized web config in NVS with defaults.");
        }
        p.end();

        // Re-open in read-only mode for safe retrieval
        if (safeBegin(p, PENDANT_PREF_NS, /*readOnly=*/true))
        {
            canRead = true;
        }
    }
    else
    {
        // Fallback: try read-only if read-write failed
        if (safeBegin(p, PENDANT_PREF_NS, /*readOnly=*/true))
        {
            canRead = true;
        }
        else
        {
            Serial.println("WARN: Couldn't open NVS for web config.");
        }
    }

    if (canRead)
    {
        String js = p.getString(PENDANT_PREF_KEY, "");
        p.end();

        if (!js.isEmpty())
        {
            DynamicJsonDocument d(2048);
            if (deserializeJson(d, js) == DeserializationError::Ok)
            {
                deserialize_config_from_json(pendant_web_cfg, d);
                Serial.println("INFO: Loaded web config from NVS.");
            }
            else
            {
                Serial.println("ERROR: Bad JSON in NVS.");
            }
        }
        else
        {
            Serial.println("INFO: No web config, using defaults.");
        }
    }

    // 3) Apply config to UI
    ui_bridge_apply_config(pendant_web_cfg);

    // 4) Load encoder (handwheel) settings
    load_encoder_config();
}

void save_pendant_configuration(const String &json_string)
{
    Preferences p;
    if (safeBegin(p, PENDANT_PREF_NS, false))
    {
        p.putString(PENDANT_PREF_KEY, json_string);
        p.end();
    }
    else
    {
        Serial.println("ERROR: Cannot save web config to NVS.");
    }
}

String get_pendant_config_as_json()
{
    DynamicJsonDocument d(2048);
    serialize_config_to_json(d, pendant_web_cfg);
    String out;
    serializeJson(d, out);
    return out;
}

void reset_pendant_to_defaults()
{
    // reset web config
    load_pendant_default_configuration();
    save_pendant_configuration(get_pendant_config_as_json());
    ui_bridge_apply_config(pendant_web_cfg);
    Serial.println("INFO: Web config reset.");

    // clear encoder NVS
    Preferences p;
    if (safeBegin(p, ENC_NS, false))
    {
        p.clear();
        p.end();
        Serial.println("INFO: Encoder config cleared.");
    }
}

//================================================================================
// INTERNAL JSON HELPERS
//================================================================================

static void load_pendant_default_configuration()
{
    // Clear any existing configuration
    pendant_web_cfg = {};

    // Basic fields
    pendant_web_cfg.num_dro_axes = 3;
    pendant_web_cfg.handwheel_enable_button = 0; // Example: first button

    // Axis labels
    pendant_web_cfg.axis_labels = {"X", "Y", "Z"};

    // Button bindings
    pendant_web_cfg.button_bindings = {
        {0, "Cycle Start", BindingType::BOUND_TO_LCNC, 101},
        {1, "Feed Hold", BindingType::BOUND_TO_LCNC, 102},
        {2, "Coolant Toggle", BindingType::BOUND_TO_BUTTON, 200}};

    // LED bindings
    pendant_web_cfg.led_bindings = {
        {0, "Spindle On", LedBindingType::LED_BOUND_TO_MATRIX, 0, 0, 0},
        {1, "Feed Override", LedBindingType::LED_BOUND_TO_MATRIX, 0, 1, 1},
        {2, "Coolant", LedBindingType::LED_BOUND_TO_BUTTON, 1, 0, 2}};

    // Macros
    pendant_web_cfg.macros = {
        {"M8 Coolant On", {"M8"}, 300},
        {"M9 Coolant Off", {"M9"}, 301}};

    Serial.println("Loaded default pendant configuration.");
}

static void deserialize_config_from_json(PendantWebConfig &cfg,
                                         const JsonDocument &doc)
{
    cfg.num_dro_axes = doc["num_dro_axes"] | cfg.num_dro_axes;
    cfg.handwheel_enable_button = doc["handwheel_enable_button"] | cfg.handwheel_enable_button;

    if (doc["axis_labels"].is<JsonArray>())
    {
        cfg.axis_labels.clear();
        for (auto v : doc["axis_labels"].as<JsonArrayConst>())
            cfg.axis_labels.push_back(v.as<const char *>());
    }

    if (doc["button_bindings"].is<JsonArray>())
    {
        cfg.button_bindings.clear();
        for (auto o : doc["button_bindings"].as<JsonArrayConst>())
        {
            cfg.button_bindings.push_back({(uint8_t)(o["button_index"] | 0),
                                           (const char *)(o["action_name"] | ""),
                                           o["type"].as<BindingType>(),
                                           (uint16_t)(o["lcnc_action_code"] | 0)});
        }
    }

    if (doc["led_bindings"].is<JsonArray>())
    {
        cfg.led_bindings.clear();
        for (auto o : doc["led_bindings"].as<JsonArrayConst>())
        {
            cfg.led_bindings.push_back({(uint8_t)(o["led_index"] | 0),
                                        (const char *)(o["signal_name"] | ""),
                                        o["type"].as<LedBindingType>(),
                                        (uint8_t)(o["matrix_index"] | 0),
                                        (uint8_t)(o["bit_index"] | 0),
                                        (uint8_t)(o["button_index"] | 0)});
        }
    }

    if (doc["macros"].is<JsonArray>())
    {
        cfg.macros.clear();
        for (auto o : doc["macros"].as<JsonArrayConst>())
        {
            MacroEntry m;
            m.name = o["name"] | "";
            m.action_code = o["action_code"] | 0;
            if (o["commands"].is<JsonArray>())
            {
                for (auto x : o["commands"].as<JsonArrayConst>())
                    m.commands.push_back(x.as<const char *>());
            }
            cfg.macros.push_back(m);
        }
    }
}

static void serialize_config_to_json(JsonDocument &doc,
                                     const PendantWebConfig &cfg)
{
    doc["num_dro_axes"] = cfg.num_dro_axes;
    doc["handwheel_enable_button"] = cfg.handwheel_enable_button;

    auto arr1 = doc["axis_labels"].to<JsonArray>();
    for (auto &s : cfg.axis_labels)
        arr1.add(s);

    auto arr2 = doc["button_bindings"].to<JsonArray>();
    for (auto &b : cfg.button_bindings)
    {
        JsonObject o = arr2.createNestedObject();
        o["button_index"] = b.button_index;
        o["action_name"] = b.action_name;
        o["type"] = b.type;
        o["lcnc_action_code"] = b.lcnc_action_code;
    }

    auto arr3 = doc["led_bindings"].to<JsonArray>();
    for (auto &l : cfg.led_bindings)
    {
        JsonObject o = arr3.createNestedObject();
        o["led_index"] = l.led_index;
        o["signal_name"] = l.signal_name;
        o["type"] = l.type;
        o["matrix_index"] = l.matrix_index;
        o["bit_index"] = l.bit_index;
        o["button_index"] = l.button_index;
    }

    auto arr4 = doc["macros"].to<JsonArray>();
    for (auto &m : cfg.macros)
    {
        JsonObject o = arr4.createNestedObject();
        o["name"] = m.name;
        o["action_code"] = m.action_code;
        auto ca = o["commands"].to<JsonArray>();
        for (auto &c : m.commands)
            ca.add(c);
    }
}
