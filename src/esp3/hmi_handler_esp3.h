/**
 * @file hmi_handler_esp3.h
 * @brief Public interface for managing all HMI peripherals on the ESP3 Handheld Pendant.
 */

#ifndef HMI_HANDLER_ESP3_H
#define HMI_HANDLER_ESP3_H

#include <stdint.h>
#include "shared_structures.h"

// Use extern "C" to make these functions available to the linker
#ifdef __cplusplus
extern "C"
{
#endif

    void hmi_pendant_init();
    void hmi_pendant_task();
    bool hmi_pendant_data_has_changed();
    void get_pendant_data(PendantStatePacket *out);
    void update_hmi_from_lcnc(const LcncStatusPacket &data);
    void get_pendant_live_status(uint32_t &btn_states, int32_t &hw_pos, uint8_t &axis_pos, uint8_t &step_pos);
    int32_t get_handwheel_diff();

#ifdef __cplusplus
}
#endif

#endif // HMI_HANDLER_ESP3_H
