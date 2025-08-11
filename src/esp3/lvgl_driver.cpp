/**
 * @file shared_structures.h
 * @brief Defines data structures for communication and configuration.
 *
 * This file contains C-compatible data packets for ESP-NOW and C++-only
 * structures for the web configuration interface.
 */

#include <stdint.h>

// --- C-Compatible Communication Packets (for ESP-NOW) ---
// NOTE: These structs use __attribute__((packed)) to prevent compiler padding,
// which is CRITICAL for reliable network communication.

/** @brief Outgoing Packet: Sent from the Pendant (HMI) to LinuxCNC. */
typedef struct
{
    uint32_t button_states;
    int32_t handwheel_position;
    float feed_override_position;
    float rapid_override_position;
    float spindle_override_position;
    uint8_t selected_axis;
    uint8_t selected_step;
} PendantStatePacket;

/** @brief Incoming Packet: Sent from LinuxCNC to the Pendant (HMI). */
typedef struct
{
    uint8_t led_matrix_states[8];
    uint32_t linuxcnc_status;
    uint16_t machine_status;
    uint16_t spindle_coolant_status;
    float feed_override;
    float rapid_override;
    float spindle_override;
    float current_tool_diameter;
    float current_feedrate;
    uint32_t spindle_rpm;
    float cutting_speed;
    float dro_pos[6];
    char macro_text[64];
} LcncStatusPacket;

// --- C++ ONLY Structures (for Web Configuration) ---
#ifdef __cplusplus

#include <string>
#include <vector>

// Defines a user-configurable macro.
struct MacroEntry
{
    std::string name;
    std::vector<std::string> commands;
    uint16_t action_code;
};

// Defines how a physical button behaves.
enum class BindingType
{
    BOUND_TO_LCNC = 0,
    BOUND_TO_BUTTON = 1,
    BOUND_TO_HANDWHEEL_ENABLE = 2
};

struct ButtonBinding
{
    uint8_t button_index;
    std::string action_name;
    BindingType type;
    uint16_t lcnc_action_code;
};

// Defines how a status LED behaves.
enum class LedBindingType
{
    LED_BOUND_TO_MATRIX = 0,
    LED_BOUND_TO_BUTTON = 1
};

struct LedBinding
{
    uint8_t led_index;
    std::string signal_name;
    LedBindingType type;
    uint8_t matrix_index;
    uint8_t bit_index;
    uint8_t button_index;
};

// The master configuration object.
struct PendantWebConfig
{
    uint8_t num_dro_axes;
    std::vector<std::string> axis_labels;
    std::vector<ButtonBinding> button_bindings;
    std::vector<LedBinding> led_bindings;
    uint8_t handwheel_enable_button;
    std::vector<MacroEntry> macros;
};

#endif // __cplusplus
