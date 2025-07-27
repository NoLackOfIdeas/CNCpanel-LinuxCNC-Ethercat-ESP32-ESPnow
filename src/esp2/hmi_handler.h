/**
 * @file hmi_handler.h
 * @brief Public interface for the HMI peripheral management module.
 *
 * This header declares the functions that the main application (`main_esp2.cpp`)
 * uses to initialize and interact with all HMI components. It abstracts the
 * underlying hardware complexity.
 */

#ifndef HMI_HANDLER_H
#define HMI_HANDLER_H

#include "shared_structures.h"

/**
 * @brief Initializes all HMI-related hardware.
 * Must be called once in the main setup() function.
 */
void hmi_init();

/**
 * @brief Main task function for the HMI handler.
 * Should be called repeatedly in the main loop() to scan buttons,
 * read joysticks, and multiplex the LED matrix.
 */
void hmi_task();

/**
 * @brief Checks if there has been a change in HMI input state.
 * @return true if an input state has changed, false otherwise.
 */
bool hmi_data_has_changed();

/**
 * @brief Fills a data structure with the current state of all HMI inputs.
 * @param data Pointer to the `struct_message_to_esp1` that will be filled.
 */
void get_hmi_data(struct_message_to_esp1 *data);

/**
 * @brief Updates the local state of the LEDs based on data received from LinuxCNC.
 * @param data The `struct_message_to_esp2` received from ESP1.
 */
void update_leds_from_lcnc(const struct_message_to_esp2 &data);

/**
 * @brief Gets the current live status of buttons and LEDs for WebSocket broadcasting.
 * @param btn_buf Pointer to a buffer to be filled with the button state bitmask.
 * @param led_buf Pointer to a buffer to be filled with the LED state bitmask.
 */
void get_live_status_data(uint8_t *btn_buf, uint8_t *led_buf);

/**
 * @brief NEW: Evaluates all user-defined action bindings based on the current machine state.
 * This is the core of the context-aware HMI logic.
 * @param lcnc_data The data structure received from ESP1 containing the machine status words.
 */
void evaluate_action_bindings(const struct_message_to_esp2 &lcnc_data);

#endif // HMI_HANDLER_H