/**
 * @file hmi_handler_esp3.cpp
 * @brief Handles all physical HMI inputs (buttons, encoders, selectors) for the ESP32-S3 Pendant.
 */

#include "hmi_handler_esp3.h"
#include "config_esp3.h"
#include "persistence_esp3.h"
#include "ui.h"
#include <Arduino.h>
#include <ESP32Encoder.h>

// --- Constants ---
static constexpr unsigned long KEY_DEBOUNCE_MS = 50;

// --- Internal State Enums and Structs ---
enum class KeyState
{
    IDLE,
    PRESSED,
    HELD,
    RELEASED
};

struct KeyInfo
{
    KeyState state = KeyState::IDLE;
    bool last_raw = false;
    unsigned long last_change_ms = 0;
};

// --- Module-static (private) variables ---
#if PENDANT_HAS_BUTTON_MATRIX
static KeyInfo key_matrix[PENDANT_MATRIX_ROWS][PENDANT_MATRIX_COLS];
#endif
static uint32_t current_button_bitmask = 0;
static ESP32Encoder handwheel;
static int32_t handwheel_position = 0;
static uint8_t selected_axis = 0;
static uint8_t selected_step = 0;
static bool data_changed_flag = false;

// --- Forward declarations for internal functions ---
static void update_keypad_states();
static void read_encoders();
static void read_selectors();

//================================================================================
// PUBLIC API FUNCTIONS (Implementations are now correctly here)
//================================================================================

void hmi_pendant_init()
{
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    handwheel.attachFullQuad(Pinout::HW_ENCODER_A, Pinout::HW_ENCODER_B);
    handwheel.clearCount();

#if PENDANT_HAS_BUTTON_MATRIX
    for (size_t i = 0; i < PENDANT_MATRIX_ROWS; ++i)
    {
        pinMode(PENDANT_ROW_PINS[i], OUTPUT);
        digitalWrite(PENDANT_ROW_PINS[i], HIGH);
    }
    for (size_t i = 0; i < PENDANT_MATRIX_COLS; ++i)
    {
        pinMode(PENDANT_COL_PINS[i], INPUT_PULLUP);
    }
#endif

#if PENDANT_HAS_LEDS
    for (size_t i = 0; i < NUM_PENDANT_LEDS; ++i)
    {
        pinMode(PENDANT_LED_PINS[i], OUTPUT);
        digitalWrite(PENDANT_LED_PINS[i], LOW);
    }
#endif
}

void hmi_pendant_task()
{
    update_keypad_states();
    read_encoders();
    read_selectors();
    ui_bridge_update_jog_selectors(selected_axis, selected_step);
}

bool hmi_pendant_data_has_changed()
{
    if (data_changed_flag)
    {
        data_changed_flag = false;
        return true;
    }
    return false;
}

void get_pendant_data(PendantStatePacket *out)
{
    if (!out)
        return;
    out->button_states = current_button_bitmask;
    out->handwheel_position = handwheel_position;
    out->selected_axis = selected_axis;
    out->selected_step = selected_step;
}

void update_hmi_from_lcnc(const LcncStatusPacket &data)
{
    // Pass the data packet to the UI bridge, which handles all screen updates.
    ui_bridge_update_from_lcnc(data);

// Handle physical LEDs based on config
#if PENDANT_HAS_LEDS
    for (size_t i = 0; i < pendant_web_cfg.led_bindings.size() && i < NUM_PENDANT_LEDS; ++i)
    {
        const auto &bind = pendant_web_cfg.led_bindings[i];
        bool on = false;
        // (Your LED logic here)
        digitalWrite(PENDANT_LED_PINS[i], on ? HIGH : LOW);
    }
#endif
}

void get_pendant_live_status(uint32_t &btn_states, int32_t &hw_pos, uint8_t &axis_pos, uint8_t &step_pos)
{
    btn_states = current_button_bitmask;
    hw_pos = handwheel_position;
    axis_pos = selected_axis;
    step_pos = selected_step;
}

int32_t get_handwheel_diff()
{
    static int32_t last_read_handwheel = 0;
    long current_pos = handwheel.getCount();
    int32_t diff = current_pos - last_read_handwheel;
    last_read_handwheel = current_pos;
    return diff;
}

//================================================================================
// INTERNAL HELPER FUNCTIONS
//================================================================================
static void update_keypad_states()
{
    // (Implementation)
}

static void read_encoders()
{
    // (Implementation)
}

static void read_selectors()
{
    // (Implementation)
}