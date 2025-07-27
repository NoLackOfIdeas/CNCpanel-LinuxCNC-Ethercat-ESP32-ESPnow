/**
 * @file hmi_handler.h
 * @brief Public interface for the HMI peripheral management module.
 *
 * This header declares the functions that the main application (`main_esp2.cpp`)
 * uses to initialize and interact with all HMI components like the button matrix,
 * joysticks, LEDs, etc. It abstracts the underlying hardware complexity, allowing
 * the main application to work with a clean and simple API.
 *
 * Concepts are derived from the project document "Integriertes Dual-ESP32-Steuerungssystem".
 */

#ifndef HMI_HANDLER_H
#define HMI_HANDLER_H

#include "shared_structures.h"

/**
 * @brief Initializes all HMI-related hardware.
 * Sets up I/O expanders, configures pin modes, and prepares all peripherals
 * for operation. This must be called once in the main setup() function.
 */
void hmi_init();

/**
 * @brief Main task function for the HMI handler.
 * This function should be called repeatedly and rapidly in the main loop().
 * It is responsible for executing all sub-tasks like scanning buttons,
 * reading joysticks, and multiplexing the LED matrix.
 */
void hmi_task();

/**
 * @brief Checks if there has been a change in HMI input state since the last check.
 * This is used to determine if a new data packet needs to be sent to ESP1
 * and if a status update should be broadcast to the web UI.
 * @return true if an input state has changed, false otherwise.
 */
bool hmi_data_has_changed();

/**
 * @brief Fills a data structure with the current state of all HMI inputs.
 * This function packs the processed data (button bitmasks, joystick values)
 * into the ESP-NOW message format for transmission to ESP1.
 * @param data Pointer to the `struct_message_to_esp1` that will be filled.
 */
void get_hmi_data(struct_message_to_esp1 *data);

/**
 * @brief Updates the local state of the LEDs based on data received from LinuxCNC.
 * This function is called from the ESP-NOW receive callback when a new packet
 * arrives from ESP1. It updates the internal buffer that the LED multiplexer uses.
 * @param data The `struct_message_to_esp2` received from ESP1 containing new LED states.
 */
void update_leds_from_lcnc(const struct_message_to_esp2 &data);

/**
 * @brief Gets the current live status of buttons and LEDs.
 * This helper function provides the main application with the current state buffers,
 * primarily for broadcasting live status updates to the web interface.
 * @param btn_buf Pointer to a buffer to be filled with the button state bitmask.
 * @param led_buf Pointer to a buffer to be filled with the LED state bitmask.
 */
void get_live_status_data(uint8_t *btn_buf, uint8_t *led_buf);

#endif // HMI_HANDLER_H