/**
 * @file hmi_handler_esp3.h
 * @brief Public interface for managing all HMI peripherals on the ESP3 Handheld Pendant.
 */

#ifndef HMI_HANDLER_ESP3_H
#define HMI_HANDLER_ESP3_H

#include <stdint.h>
#include "shared_structures.h" // For packet struct definitions

/**
 * @brief Initializes all pendant HMI peripherals (GPIOs, encoders).
 */
void hmi_pendant_init();

/**
 * @brief Periodic HMI task handler to be called in the main loop.
 */
void hmi_pendant_task();

/**
 * @brief Checks if any pendant input state has changed since the last check.
 * @return true if data has changed, false otherwise.
 */
bool hmi_pendant_data_has_changed();

/**
 * @brief Populates an outgoing packet with the current pendant state.
 * @param[out] out A pointer to the packet to be filled.
 */
void get_pendant_data(PendantStatePacket *out);

/**
 * @brief Dispatches inbound status from LinuxCNC to the HMI (physical LEDs and UI).
 * @param data The status packet received from LinuxCNC.
 */
void update_hmi_from_lcnc(const LcncStatusPacket &data);

/**
 * @brief Retrieves the live hardware status for other modules (e.g., web interface).
 */
void get_pendant_live_status(uint32_t &btn_states, int32_t &hw_pos, uint8_t &axis_pos, uint8_t &step_pos);

/**
 * @brief Gets the accumulated handwheel rotation since the last query.
 * @return The signed difference in encoder counts.
 */
int32_t get_handwheel_diff();

#endif // HMI_HANDLER_ESP3_H