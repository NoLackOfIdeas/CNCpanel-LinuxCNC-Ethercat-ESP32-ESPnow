/**
 * @file config_esp3.cpp
 * @brief Defines and loads the default software configuration for the pendant.
 */

#include "persistence_esp3.h" // Includes the declaration for pendant_web_cfg
#include "shared_structures.h"
#include <Arduino.h>

// This is the one-and-only definition of the global configuration object.
PendantWebConfig pendant_web_cfg;

// --- Default Configuration Values ---

void load_pendant_default_configuration()
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