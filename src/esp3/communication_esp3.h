#ifndef COMMUNICATION_ESP3_H
#define COMMUNICATION_ESP3_H

#include "shared_structures.h"

/**
 * Initialize ESP-NOW (or your chosen transport) and ready the module
 * for send/receive.
 */
void communication_esp3_init();

/**
 * Call periodically from loop() if your transport needs polling.
 * For ESP-NOW this can be a no-op.
 */
void communication_esp3_loop();

/**
 * Send the current HMI → LCNC packet over ESP-NOW.
 * Returns true on success.
 */
bool communication_esp3_send(const struct_message_from_esp3 &msg);

/**
 * Register a callback that will fire whenever an incoming
 * LCNC → HMI packet arrives.
 */
void communication_esp3_register_receive_callback(
    void (*cb)(const struct_message_to_hmi &msg));

/**
 * Send a single LCNC action (button, macro, handwheel‐enable toggle).
 */
void lcnc_send_action(uint16_t action_code, bool pressed);

bool communication_esp3_receive(struct_message_to_hmi &msg);

#endif // COMMUNICATION_ESP3_H
