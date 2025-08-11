/**
 * @file persistence_esp3.h
 * @brief Public interface for loading and saving the pendant's configuration.
 *
 * This module handles the serialization of the configuration object to and from
 * the ESP32's non-volatile storage (NVS) using JSON.
 */

#pragma once

#include "shared_structures.h"
#include <Arduino.h> // For the String class

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief The global runtime configuration object for the entire application.
     *
     * It is defined and managed in persistence_esp3.cpp. Other modules can
     * access it by including this header file.
     */
    extern PendantWebConfig pendant_web_cfg;

    /**
     * @brief Loads the configuration from NVS into the global `pendant_web_cfg` object.
     *
     * If no configuration is found in NVS, it loads and uses the default values.
     */
    void load_pendant_configuration();

    /**
     * @brief Saves a configuration from a JSON string into NVS.
     * @param json_string A string containing the configuration in JSON format.
     */
    void save_pendant_configuration(const String &json_string);

    /**
     * @brief Serializes the current in-memory `pendant_web_cfg` into a JSON string.
     * @return A String object containing the full configuration in JSON format.
     */
    String get_pendant_config_as_json();

    /**
     * @brief Resets the configuration to factory defaults and saves them to NVS.
     */
    void reset_pendant_to_defaults();

    void update_hmi_from_config();

    /**
     * @brief A stub function for a factory test routine.
     */
    void run_factory_test();

#ifdef __cplusplus
}
#endif