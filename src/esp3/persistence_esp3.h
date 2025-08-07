#pragma once

/**
 * persistence_esp3.h
 *
 * Load and save the PendantWebConfig to non-volatile storage (NVS) on the ESP32.
 * The web UI exchanges JSON payloads matching the fields of PendantWebConfig.
 */

#include <Arduino.h>
#include "shared_structures.h"

/**
 * Global runtime configuration object.
 * Defined in config_esp3.cpp; populated by load_pendant_configuration().
 */
extern PendantWebConfig pendant_web_cfg;

/**
 * Load the JSON-serialized PendantWebConfig from NVS.
 * On success, populates pendant_web_cfg; on failure, leaves defaults in place.
 */
void load_pendant_configuration();

/**
 * Save a JSON-serialized PendantWebConfig into NVS.
 * The json_string must represent fields matching PendantWebConfig.
 */
void save_pendant_configuration(const String &json_string);

/**
 * Serialize the current pendant_web_cfg back into a JSON string
 * suitable for sending to the web UI.
 */
String get_pendant_config_as_json();

/**
 * Run the built-in factory test routine (LEDs, buttons, sensors…).
 */
void run_factory_test();

/**
 * Reset the pendant configuration to default values and persist them.
 * This will overwrite any existing configuration in NVS.
 */
void reset_pendant_to_defaults();

/**
 * Load the default in-memory configuration (button + LED bindings).
 * Used internally by both load_pendant_configuration() on failure
 * and reset_pendant_to_defaults().
 */
void load_pendant_default_configuration();
