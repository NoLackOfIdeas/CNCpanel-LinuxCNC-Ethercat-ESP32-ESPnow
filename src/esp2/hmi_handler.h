#ifndef HMI_HANDLER_H
#define HMI_HANDLER_H

#include "shared_structures.h"

void hmi_init();
void hmi_task();

uint64_t hmi_get_button_states();
uint64_t hmi_get_previous_button_states();

void hmi_set_led_states(uint64_t states);

bool hmi_data_has_changed();
void hmi_set_data_changed_flag();

void hmi_get_full_hmi_data(struct_message_to_esp1 *data);

#endif // HMI_HANDLER_H