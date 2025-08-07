// config_esp3.cpp
#include "config_esp3.h"
#include "shared_structures.h"
#include <Arduino.h> // for Serial.println()

PendantWebConfig pendant_web_cfg; // actual definition

// Default constants
static constexpr int DEFAULT_NUM_DRO_AXES = 3;
static constexpr int DEFAULT_HANDWHEEL_BTN = 0;
static constexpr char DEFAULT_AXIS_NAMES[] = {'X', 'Y', 'Z', 'A', 'B', 'C'};

// Default buttons
static const ButtonBinding DEFAULT_BUTTON_BINDINGS[] = {
    {0, "Cycle Start", BOUND_TO_LCNC, 101},
    {1, "Feed Hold", BOUND_TO_LCNC, 102},
    {2, "Coolant Toggle", BOUND_TO_BUTTON, 200}};

// Default LEDs
static const LedBinding DEFAULT_LED_BINDINGS[] = {
    {0, "Spindle On", LED_BOUND_TO_MATRIX, 0, 0, 0},
    {1, "Feed Override", LED_BOUND_TO_MATRIX, 0, 1, 1},
    {2, "Coolant", LED_BOUND_TO_BUTTON, 1, 0, 2}};

void load_pendant_default_configuration()
{
    // Basic fields
    pendant_web_cfg.num_dro_axes = DEFAULT_NUM_DRO_AXES;
    pendant_web_cfg.handwheel_enable_button = DEFAULT_HANDWHEEL_BTN;

    // Axis labels
    pendant_web_cfg.axis_labels.clear();
    for (int i = 0; i < DEFAULT_NUM_DRO_AXES; ++i)
    {
        pendant_web_cfg.axis_labels.push_back(std::string(1, DEFAULT_AXIS_NAMES[i]));
    }

    // Button bindings
    pendant_web_cfg.button_bindings.clear();
    for (auto &b : DEFAULT_BUTTON_BINDINGS)
        pendant_web_cfg.button_bindings.push_back(b);

    // LED bindings
    pendant_web_cfg.led_bindings.clear();
    for (auto &l : DEFAULT_LED_BINDINGS)
        pendant_web_cfg.led_bindings.push_back(l);

    // Macros
    pendant_web_cfg.macros.clear();

    {
        MacroEntry m;
        m.name = "M8 Coolant On";
        m.commands = {"M8"};
        m.action_code = 300;
        pendant_web_cfg.macros.push_back(m);
    }
    {
        MacroEntry m;
        m.name = "M9 Coolant Off";
        m.commands = {"M9"};
        m.action_code = 301;
        pendant_web_cfg.macros.push_back(m);
    }

    Serial.println("Loaded default pendant configuration.");
}
