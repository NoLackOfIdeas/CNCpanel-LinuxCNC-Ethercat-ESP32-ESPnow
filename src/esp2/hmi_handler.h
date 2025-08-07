/**
 * @file hmi_handler.h
 * @brief Public interface for the Main HMI Panel's peripheral management module (ESP2).
 *
 * This header declares the functions that the main application (`main_esp2.cpp`)
 * uses to initialize and interact with all of the main panel's components, including
 * the 8x8 button matrix, joysticks, LEDs, and local encoders. It abstracts the
 * underlying hardware complexity.
 *
 * Concepts are derived from the project document "Integriertes Dual-ESP32-Steuerungssystem".
 */

#ifndef HMI_HANDLER_H
#define HMI_HANDLER_H

#include "shared_structures.h" // Includes the correct data structure definitions for the 3-node network

// --- FUNCTION PROTOTYPES ---

/**
 * @brief Initializes all HMI-related hardware on the main panel.
 * Sets up I/O expanders, configures pin modes, and prepares all peripherals
 * for operation. This must be called once in the main setup() function.
 */
void hmi_init();

/**
 * @brief Main task function for the HMI handler.
 * This function should be called repeatedly and rapidly in the main loop().
 * It is responsible for executing all sub-tasks like scanning buttons,
 * reading joysticks and encoders, and multiplexing the LED matrix.
 */
void hmi_task();

/**
 * @brief Checks if there has been a change in the main panel's input state.
 * This is used to determine if a new data packet needs to be sent to ESP1.
 * @return true if an input state has changed since the last check, false otherwise.
 */
bool hmi_data_has_changed();

/**
 * @brief Fills a data structure with the current state of all main panel inputs.
 * This function packs the processed data (button bitmasks, joystick values)
 * into the ESP-NOW message format for transmission to ESP1.
 * @param data Pointer to the `struct_message_from_esp2` that will be filled.
 */
void get_hmi_data(struct_message_from_esp2 *data);

/**
 * @brief Updates the local state of the main panel's LEDs based on data received from LinuxCNC.
 * This function is called from the ESP-NOW receive callback when a new packet
 * arrives from ESP1. It updates the internal buffer that the LED multiplexer uses.
 * @param data The `struct_message_to_hmi` received from ESP1.
 */
void update_leds_from_lcnc(const struct_message_to_hmi &data);

/**
 * @brief Gets the current live status of buttons and LEDs for WebSocket broadcasting.
 * @param btn_buf Pointer to a buffer to be filled with the button state bitmask.
 * @param led_buf Pointer to a buffer to be filled with the LED state bitmask.
 */
void get_live_status_data(uint8_t *btn_buf, uint8_t *led_buf);

/**
 * @brief Evaluates all user-defined action bindings based on the current machine state.
 * This is the core of the context-aware HMI logic, allowing the panel to react
 * to states like "in jog mode" or "program running".
 * @param lcnc_data The data structure received from ESP1 containing the machine status words.
 */
void evaluate_action_bindings(const struct_message_to_hmi &lcnc_data);

#endif // HMI_HANDLER_H