/**
 * @file hmi_handler_esp3.cpp
 * @brief Handles GUI/HMI updates and UI synchronization for the ESP32-S3 Pendant.
 *        No persistence logic here – purely hardware and UI.
 */

#include "lv_conf.h"
#include "config_esp3.h"
#include "hmi_handler_esp3.h"
#include "shared_structures.h"
#include <Arduino.h>
#include <ESP32Encoder.h>
#include <lvgl.h>
#include "ui.h"

static constexpr unsigned long KEY_DEBOUNCE_MS = 50;

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

extern PendantWebConfig pendant_web_cfg;
extern lv_group_t *g_input_group;

extern const uint8_t PENDANT_ROW_PINS[];
extern const uint8_t PENDANT_COL_PINS[];
extern const uint8_t PENDANT_LED_PINS[];
extern const int NUM_PENDANT_LEDS;

static KeyInfo key_matrix[PENDANT_MATRIX_ROWS][PENDANT_MATRIX_COLS];
static uint32_t current_button_bitmask = 0;
static ESP32Encoder handwheel;

#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
static ESP32Encoder feed_override_encoder;
static int32_t feed_override_position = 0;
#endif

#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
static ESP32Encoder rapid_override_encoder;
static int32_t rapid_override_position = 0;
#endif

#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
static ESP32Encoder spindle_override_encoder;
static int32_t spindle_override_position = 0;
#endif

static int32_t handwheel_position = 0;
static int32_t last_read_handwheel = 0;
static uint8_t selected_axis = 0;
static uint8_t selected_step = 0;
static bool data_changed_flag = false;

static void dispatch_button_action(int idx, bool pressed);
static void update_keypad_states();
static void read_encoders();
static void read_selectors();

//--------------------------------------------------------------------------------
// Scan button matrix with debounce and dispatch actions
//--------------------------------------------------------------------------------
static void update_keypad_states()
{
#if PENDANT_HAS_BUTTON_MATRIX
    unsigned long now = millis();

    for (int row = 0; row < PENDANT_MATRIX_ROWS; ++row)
    {
        digitalWrite(PENDANT_ROW_PINS[row], LOW);

        for (int col = 0; col < PENDANT_MATRIX_COLS; ++col)
        {
            int idx = row * PENDANT_MATRIX_COLS + col;
            if (idx >= (int)pendant_web_cfg.button_bindings.size())
                continue;

            bool raw = (digitalRead(PENDANT_COL_PINS[col]) == LOW);
            KeyInfo &k = key_matrix[row][col];

            if (raw != k.last_raw)
            {
                k.last_raw = raw;
                k.last_change_ms = now;
            }
            if (now - k.last_change_ms < KEY_DEBOUNCE_MS)
                continue;

            KeyState old = k.state;
            switch (k.state)
            {
            case KeyState::IDLE:
                k.state = raw ? KeyState::PRESSED : KeyState::IDLE;
                break;
            case KeyState::PRESSED:
                k.state = raw ? KeyState::HELD : KeyState::RELEASED;
                break;
            case KeyState::HELD:
                k.state = raw ? KeyState::HELD : KeyState::RELEASED;
                break;
            case KeyState::RELEASED:
                k.state = raw ? KeyState::PRESSED : KeyState::IDLE;
                break;
            }

            if (k.state != old)
            {
                if (k.state == KeyState::PRESSED || k.state == KeyState::HELD)
                    bitSet(current_button_bitmask, idx);
                else
                    bitClear(current_button_bitmask, idx);

                data_changed_flag = true;

                if (k.state == KeyState::PRESSED || k.state == KeyState::RELEASED)
                    dispatch_button_action(idx, k.state == KeyState::PRESSED);
            }
        }

        digitalWrite(PENDANT_ROW_PINS[row], HIGH);
    }
#endif
}

//--------------------------------------------------------------------------------
// Read all encoders and flag changes
//--------------------------------------------------------------------------------
static void read_encoders()
{
    long hw = handwheel.getCount();
    if (hw != handwheel_position)
    {
        handwheel_position = hw;
        data_changed_flag = true;
    }

#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
    long fo = feed_override_encoder.getCount();
    if (fo != feed_override_position)
    {
        feed_override_position = fo;
        data_changed_flag = true;
    }
#endif

#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
    long ro = rapid_override_encoder.getCount();
    if (ro != rapid_override_position)
    {
        rapid_override_position = ro;
        data_changed_flag = true;
    }
#endif

#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
    long so = spindle_override_encoder.getCount();
    if (so != spindle_override_position)
    {
        spindle_override_position = so;
        data_changed_flag = true;
    }
#endif
}

//--------------------------------------------------------------------------------
// Read analog selectors for axis and step
//--------------------------------------------------------------------------------
static void read_selectors()
{
    uint8_t old_axis = selected_axis;
    uint8_t old_step = selected_step;

    int adc_axis = analogRead(PIN_AXIS_SELECTOR_ADC);
    int adc_step = analogRead(PIN_STEP_SELECTOR_ADC);

    // Map ADC to discrete axis
    if (adc_axis < 200)
        selected_axis = 6;
    else if (adc_axis < 600)
        selected_axis = 0;
    else if (adc_axis < 1000)
        selected_axis = 1;
    else if (adc_axis < 1400)
        selected_axis = 2;
    else if (adc_axis < 1800)
        selected_axis = 3;
    else if (adc_axis < 2200)
        selected_axis = 4;
    else
        selected_axis = 5;

    // Map ADC to step increment
    if (adc_step < 500)
        selected_step = 0;
    else if (adc_step < 1500)
        selected_step = 1;
    else if (adc_step < 2500)
        selected_step = 2;
    else
        selected_step = 3;

    if (old_axis != selected_axis || old_step != selected_step)
        data_changed_flag = true;
}

//--------------------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------------------

void hmi_pendant_init()
{
    // NO persistence here!
    // Just wire up encoders, pins, and push any on-screen tweaks
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    handwheel.attachFullQuad(PIN_HW_ENCODER_A, PIN_HW_ENCODER_B);
    handwheel.clearCount();

#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
    feed_override_encoder.attachFullQuad(PIN_FEED_OVR_A, PIN_FEED_OVR_B);
    feed_override_position = feed_override_encoder.getCount();
#endif

#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
    rapid_override_encoder.attachFullQuad(PIN_RAPID_OVR_A, PIN_RAPID_OVR_B);
    rapid_override_position = rapid_override_encoder.getCount();
#endif

#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
    spindle_override_encoder.attachFullQuad(PIN_SPINDLE_OVR_A, PIN_SPINDLE_OVR_B);
    spindle_override_position = spindle_override_encoder.getCount();
#endif

#if PENDANT_HAS_BUTTON_MATRIX
    for (int i = 0; i < PENDANT_MATRIX_ROWS; ++i)
    {
        pinMode(PENDANT_ROW_PINS[i], OUTPUT);
        digitalWrite(PENDANT_ROW_PINS[i], HIGH);
    }
    for (int i = 0; i < PENDANT_MATRIX_COLS; ++i)
    {
        pinMode(PENDANT_COL_PINS[i], INPUT_PULLUP);
    }
#endif

#if PENDANT_HAS_LEDS
    for (int i = 0; i < NUM_PENDANT_LEDS; ++i)
    {
        pinMode(PENDANT_LED_PINS[i], OUTPUT);
        digitalWrite(PENDANT_LED_PINS[i], LOW);
    }
#endif

    // Push configuration into on-screen widgets
    update_hmi_from_config();
}

void hmi_pendant_task()
{
    update_keypad_states();
    read_encoders();
    read_selectors();
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

void get_pendant_data(struct_message_from_esp3 *out)
{
    out->button_states = current_button_bitmask;
    out->handwheel_position = handwheel_position;
#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
    out->feed_override_position = feed_override_position;
#endif
#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
    out->rapid_override_position = rapid_override_position;
#endif
#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
    out->spindle_override_position = spindle_override_position;
#endif
    out->selected_axis = selected_axis;
    out->selected_step = selected_step;
}

void update_hmi_from_lcnc(const struct_message_to_hmi &data)
{
#if PENDANT_HAS_LEDS
    for (size_t i = 0; i < pendant_web_cfg.led_bindings.size() && i < (size_t)NUM_PENDANT_LEDS; ++i)
    {
        const auto &bind = pendant_web_cfg.led_bindings[i];
        bool on = false;
        switch (bind.type)
        {
        case BOUND_TO_LCNC:
            on = bitRead(data.led_matrix_states[bind.matrix_index], bind.bit_index);
            break;
        case BOUND_TO_BUTTON:
            on = bitRead(current_button_bitmask, bind.button_index);
            break;
        case BOUND_TO_HANDWHEEL_ENABLE:
            on = bitRead(current_button_bitmask, pendant_web_cfg.handwheel_enable_button);
            break;
        }
        digitalWrite(PENDANT_LED_PINS[i], on ? HIGH : LOW);
    }
#endif

    ui_update_jog_screen(data, selected_axis, selected_step);
    ui_update_macro_screen(data);
    lv_timer_handler();
}

int32_t get_handwheel_diff()
{
    long curr = handwheel.getCount();
    int32_t diff = curr - last_read_handwheel;
    last_read_handwheel = curr;
    return diff;
}

void get_pendant_live_status(uint32_t &btn_states,
                             int32_t &hw_pos,
                             uint8_t &axis_pos,
                             uint8_t &step_pos)
{
    btn_states = current_button_bitmask;
    hw_pos = handwheel_position;
    axis_pos = selected_axis;
    step_pos = selected_step;
}

static void dispatch_button_action(int idx, bool pressed)
{
    if (idx >= (int)pendant_web_cfg.button_bindings.size())
        return;

    const auto &btn = pendant_web_cfg.button_bindings[idx];
    switch (btn.type)
    {
    case BOUND_TO_LCNC:
        lcnc_send_action(btn.lcnc_action_code, pressed);
        break;
    case BOUND_TO_BUTTON:
        for (size_t i = 0; i < pendant_web_cfg.led_bindings.size() && i < (size_t)NUM_PENDANT_LEDS; ++i)
        {
            const auto &ld = pendant_web_cfg.led_bindings[i];
            if (ld.type == LED_BOUND_TO_BUTTON && ld.button_index == idx)
                digitalWrite(PENDANT_LED_PINS[i], pressed ? HIGH : LOW);
        }
        break;
    case BOUND_TO_HANDWHEEL_ENABLE:
        lcnc_send_action(0xF0, pressed);
        break;
    }
}

void update_hmi_from_config()
{
    auto &cfg = PendantConfig::current;
    ui_set_dro_axis_count(cfg.num_dro_axes);
    ui_set_handwheel_enable_button(cfg.handwheel_enable_button);
    ui_configure_button_bindings(cfg.button_bindings);
    ui_configure_led_bindings(cfg.led_bindings);
    ui_configure_macros(cfg.macros);
}
