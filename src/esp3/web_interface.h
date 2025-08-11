/**
 * @file web_interface.h
 * @brief Public interface for the Web Server and WebSocket module.
 */

#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include "shared_structures.h"
#include <stdint.h>

/**
 * @brief Initializes the web server, WebSocket, and OTA endpoints.
 */
void web_interface_init();

/**
 * @brief Handles periodic tasks for the web server, like cleaning up clients.
 */
void web_interface_loop();

/**
 * @brief Broadcasts the full LCNC status to all connected WebSocket clients.
 * @param data The incoming data from LinuxCNC.
 */
void web_interface_broadcast_status(const LcncStatusPacket &data);

/**
 * @brief Broadcasts the live pendant hardware status to all WebSocket clients.
 * @param btn_states Bitmask of pressed buttons.
 * @param hw_pos Current handwheel position.
 * @param axis_pos Current axis selector position.
 * @param step_pos Current step selector position.
 */
void web_interface_broadcast_live_pendant_status(uint32_t btn_states, int32_t hw_pos, uint8_t axis_pos, uint8_t step_pos);

#endif // WEB_INTERFACE_H