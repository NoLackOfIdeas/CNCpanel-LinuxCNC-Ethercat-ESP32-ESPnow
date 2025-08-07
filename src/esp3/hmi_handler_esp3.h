/**
 * @file hmi_handler_esp3.h
 * @brief Public interface for managing all HMI peripherals on the ESP3 Handheld Pendant.
 *
 * This header declares routines to initialize and poll the pendant’s
 * inputs (buttons, handwheel, selectors), prepare outbound ESP-NOW payloads,
 * and dispatch inbound LinuxCNC status updates to the LVGL UI and LEDs.
 */

#ifndef HMI_HANDLER_ESP3_H
#define HMI_HANDLER_ESP3_H

#include <stdint.h>
#include "shared_structures.h" // struct_message_to_hmi, struct_message_from_esp3
#include "ui.h"                // ui_update_jog_screen, ui_update_macro_screen, ui_switch_to_screen

/**
 * @brief Initialize all pendant HMI peripherals.
 *
 * - Configures GPIOs for buttons, selectors, LEDs
 * - Sets up the rotary handwheel encoder
 * - Prepares the LVGL input group for touchscreen/list widgets
 *
 * Call once in setup().
 */
void hmi_pendant_init();

/**
 * @brief Periodic HMI task handler.
 *
 * - Scans buttons, handwheel, axis/step selectors
 * - Debounces inputs and updates internal state
 * - Should be called in the main loop before lv_timer_handler()
 */
void hmi_pendant_task();

/**
 * @brief Query whether any pendant input state has changed since the last call.
 *
 * @return true if buttons, handwheel, or selectors have new data
 */
bool hmi_pendant_data_has_changed();

/**
 * @brief Package current pendant inputs for ESP-NOW transmission.
 *
 * Fills the provided struct with:
 *  - Button bitmask
 *  - Handwheel delta since last get
 *  - Axis and step selector positions
 *
 * @param[out] data  Pointer to the outbound message to populate
 */
void get_pendant_data(struct_message_from_esp3 *data);

/**
 * @brief Dispatch inbound status from LinuxCNC to the HMI.
 *
 * Updates the LVGL UI (jog or macro screen) and any status LEDs
 * according to the fields in @p data.
 *
 * - If the current UI screen is jog, calls ui_update_jog_screen(...)
 * - If the current UI screen is macro, calls ui_update_macro_screen(...)
 *
 * @param data  Status struct received over ESP-NOW from LinuxCNC
 */
void update_hmi_from_lcnc(const struct_message_to_hmi &data);

/**
 * @brief (Re)apply button/encoder/LED bindings after a config change.
 *
 * Called once during setup() and again at runtime whenever the
 * user saves or resets the config via the Web UI.
 */
void update_hmi_from_config();

/**
 * @brief Get the accumulated handwheel rotation since the last query.
 *
 * @return Signed count difference (can be positive or negative)
 */
int32_t get_handwheel_diff();

/**
 * @brief Retrieve live pendant state for web/status interfaces.
 *
 * @param[out] btn_states  Bitmask of pressed buttons
 * @param[out] hw_pos      Current handwheel encoder absolute count
 * @param[out] axis_pos    Current axis selector position (0…5)
 * @param[out] step_pos    Current step selector position (0…N-1)
 */
void get_pendant_live_status(uint32_t &btn_states,
                             int32_t &hw_pos,
                             uint8_t &axis_pos,
                             uint8_t &step_pos);

#endif // HMI_HANDLER_ESP3_H
