#pragma once

#include <stdint.h>
#include <vector>
#include <string>

//
// Macro definitions for UI macro list
//
struct MacroEntry
{
    // Display name shown in the UI
    std::string name;

    // One command per line for the preview
    std::vector<std::string> commands;

    // Used to trigger macro via button
    uint16_t action_code;
};

//
// MAC addresses for ESP-NOW (override with your actual boards’ MACs)
//
static uint8_t esp1_mac_address[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01};
static uint8_t esp2_mac_address[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x02};
static uint8_t esp3_mac_address[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x03};

//
// HMI → LCNC (outgoing) packet
//
typedef struct
{
    uint32_t button_states;     // bitmask for up to 32 pendant buttons
    int32_t handwheel_position; // raw count
    float feed_override_position;
    float rapid_override_position;
    float spindle_override_position;
    uint8_t selected_axis; // 0–6
    uint8_t selected_step; // 0–3
} struct_message_from_esp3;

//
// LCNC → HMI (incoming) packet
//
typedef struct
{
    uint8_t led_matrix_states[8]; // 8×8 LED matrix bitmask
    uint32_t linuxcnc_status;
    uint16_t machine_status;
    uint16_t spindle_coolant_status;
    float feed_override;
    float rapid_override;
    float spindle_override;
    float current_tool_diameter;
    float current_feedrate;
    uint32_t spindle_rpm;
    float cutting_speed; // newly added
    float dro_pos[6];
    char macro_text[64]; // macro preview text
} struct_message_to_hmi;

#ifdef __cplusplus
extern "C"
{
#endif

    // Comm layer API
    void communication_esp3_init();
    void communication_esp3_loop();
    bool communication_esp3_send(const struct_message_from_esp3 &msg);
    bool communication_esp3_receive(struct_message_to_hmi &msg);
    void communication_esp3_register_receive_callback(void (*cb)(const struct_message_to_hmi &));
    void lcnc_send_action(uint16_t action_code, bool pressed);

#ifdef __cplusplus
} // extern "C"

//
// WEB‐CONFIG TYPES FOR THE PENDANT UI
//

// Binding behavior types
enum BindingType
{
    BOUND_TO_LCNC = 0,
    BOUND_TO_BUTTON = 1,
    BOUND_TO_HANDWHEEL_ENABLE = 2
};

// LED binding types
enum LedBindingType
{
    LED_BOUND_TO_MATRIX = 0,
    LED_BOUND_TO_BUTTON = 1
};

// Single‐LED binding
struct LedBinding
{
    uint8_t led_index;       // index shown in the UI list
    std::string signal_name; // human‐readable label

    // Runtime behavior
    LedBindingType type;
    uint8_t matrix_index;
    uint8_t bit_index;
    uint8_t button_index;
};

// Single‐button binding
struct ButtonBinding
{
    uint8_t button_index;    // index shown in the UI list
    std::string action_name; // human‐readable label

    // Runtime behavior
    BindingType type;
    uint16_t lcnc_action_code;
};

// Master dynamic web configuration
struct PendantWebConfig
{
    uint8_t num_dro_axes;                       // “1..6” from web UI
    std::vector<std::string> axis_labels;       // labels for each DRO axis
    std::vector<ButtonBinding> button_bindings; // all panel buttons
    std::vector<LedBinding> led_bindings;       // all status LEDs
    uint8_t handwheel_enable_button;            // which button toggles handwheel
    std::vector<MacroEntry> macros;             // user‐defined macros
};
#endif // __cplusplus
