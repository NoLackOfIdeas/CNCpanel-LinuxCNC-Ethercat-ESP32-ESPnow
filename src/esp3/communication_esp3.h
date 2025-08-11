/**
 * @file communication_esp3.h
 * @brief Public API for the ESP-NOW communication layer.
 */

#ifndef COMMUNICATION_ESP3_H
#define COMMUNICATION_ESP3_H

#include "shared_structures.h" // For packet struct definitions

/**
 * @brief Initializes the ESP-NOW service. Must be called once from setup().
 */
void communication_esp3_init();

/**
 * @brief Sends the pendant's state packet over ESP-NOW.
 * @param msg The PendantStatePacket to send.
 * @return true if the message was successfully queued for sending, false otherwise.
 */
bool communication_esp3_send(const PendantStatePacket &msg);

/**
 * @brief Registers a function to be called when a status packet arrives from LCNC.
 * @param cb The callback function pointer.
 */
void communication_esp3_register_receive_callback(void (*cb)(const LcncStatusPacket &msg));

#endif // COMMUNICATION_ESP3_H