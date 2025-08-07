/**
 * @file hmi_handler.cpp
 * @brief Implements the core logic for managing all HMI peripherals for the Main Panel (ESP2).
 *
 * This file contains the state machine for debouncing the button matrix,
 * the processing pipeline for the analog joysticks, the multiplexing
 * logic for driving the physical LED matrix, and the evaluation logic for
 * user-defined action bindings.
 */

#include "hmi_handler.h"
#include "config_esp2.h"
#include "persistence.h"
#include <Adafruit_MCP23X17.h>
#include <SPI.h>
#include <ESP32Encoder.h>

// --- CONFIGURATION & CONSTANTS ---
const unsigned long KEY_DEBOUNCE_MS = 50;

// --- INTERNAL DATA STRUCTURES ---
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
};

// --- GLOBAL OBJECTS AND STATE VARIABLES ---
Adafruit_MCP23X17 mcp_buttons;
Adafruit_MCP23X17 mcp_led_rows;
Adafruit_MCP23X17 mcp_led_cols;
ESP32Encoder encoders[NUM_ENCODERS_ESP2];
extern WebConfig web_cfg;
static KeyInfo key_matrix[MATRIX_ROWS][MATRIX_COLS];
static uint8_t current_button_bitmask[MATRIX_ROWS] = {0};
static uint8_t current_led_states[MATRIX_ROWS] = {0};
static int16_t processed_joystick_values[NUM_JOYSTICKS][NUM_JOYSTICK_AXES];
static int32_t hmi_encoder_values[NUM_ENCODERS_ESP2] = {0};
static bool joystick_axis_locked[NUM_JOYSTICKS][NUM_JOYSTICK_AXES] = {false};
static bool data_changed_flag = false;
static uint8_t current_led_scan_col = 0;
static bool joystick_1_enabled = true;

// --- PRIVATE FUNCTIONS: CORE LOGIC ---

/**
 * @brief Drives the physical LED matrix using multiplexing.
 */
void update_led_matrix()
{
    mcp_led_cols.writeGPIOAB(0x0000);
    uint16_t row_data = 0;
    for (int i = 0; i < MATRIX_ROWS; i++)
    {
        if (bitRead(current_led_states[i], current_led_scan_col))
        {
            bitSet(row_data, i);
        }
    }
    mcp_led_rows.writeGPIOAB(row_data);
    mcp_led_cols.digitalWrite(current_led_scan_col, HIGH);
    current_led_scan_col = (current_led_scan_col + 1) % MATRIX_COLS;
}

/**
 * @brief Scans the button matrix and updates the state of each key.
 */
void update_keypad_states()
{
    bool state_has_changed = false;
    for (int row = 0; row < MATRIX_ROWS; row++)
    {
        mcp_buttons.digitalWrite(row, LOW);
        for (int col = 0; col < MATRIX_COLS; col++)
        {
            int button_idx = row * MATRIX_COLS + col;
            if (button_idx >= MAX_BUTTONS_DEFINED)
                continue;

            bool is_pressed = (mcp_buttons.digitalRead(col + 8) == LOW);
            KeyInfo &key = key_matrix[row][col];
            KeyState old_state = key.state;

            switch (key.state)
            {
            case KeyState::IDLE:
                if (is_pressed)
                    key.state = KeyState::PRESSED;
                break;
            case KeyState::PRESSED:
                key.state = is_pressed ? KeyState::HELD : KeyState::RELEASED;
                break;
            case KeyState::HELD:
                if (!is_pressed)
                    key.state = KeyState::RELEASED;
                break;
            case KeyState::RELEASED:
                key.state = KeyState::IDLE;
                break;
            }

            if (key.state != old_state)
            {
                state_has_changed = true;
                if (key.state == KeyState::PRESSED)
                {
                    bitSet(current_button_bitmask[row], col);
                    const auto &btn_cfg = button_configs[button_idx];
                    if (btn_cfg.extra_action == ButtonExtraAction::TOGGLE_JOYSTICK_AXIS_LOCK)
                    {
                        joystick_axis_locked[btn_cfg.target_joystick_index][btn_cfg.target_axis_index] = !joystick_axis_locked[btn_cfg.target_joystick_index][btn_cfg.target_axis_index];
                    }
                }
                else if (key.state == KeyState::RELEASED)
                {
                    bitClear(current_button_bitmask[row], col);
                }
            }
        }
        mcp_buttons.digitalWrite(row, HIGH);
    }
    if (state_has_changed)
        data_changed_flag = true;
}

/**
 * @brief Reads all potentiometers and processes them.
 */
void process_joysticks()
{
    if (!joystick_1_enabled)
    {
        for (int j = 0; j < NUM_JOYSTICK_AXES; j++)
        {
            processed_joystick_values[0][j] = 0;
        }
    }
    else
    {
        for (int i = 0; i < NUM_JOYSTICKS; i++)
        {
            for (int j = 0; j < NUM_JOYSTICK_AXES; j++)
            {
                if (joystick_axis_locked[i][j])
                {
                    processed_joystick_values[i][j] = 0;
                    continue;
                }
                const auto &static_cfg = joystick_configs[i].axes[j];
                const auto &dynamic_cfg = web_cfg.joysticks[i][j];
                int raw_value = analogRead(POTI_PINS[static_cfg.poti_index]);
                int center_value = 2048;
                if (abs(raw_value - center_value) < dynamic_cfg.center_deadzone)
                {
                    processed_joystick_values[i][j] = 0;
                    continue;
                }
                long mapped_value = (raw_value > center_value)
                                        ? map(raw_value, center_value + dynamic_cfg.center_deadzone, 4095, 0, 512)
                                        : map(raw_value, 0, center_value - dynamic_cfg.center_deadzone, -512, 0);
                mapped_value *= dynamic_cfg.sensitivity;
                if (dynamic_cfg.is_inverted)
                {
                    mapped_value *= -1;
                }
                processed_joystick_values[i][j] = constrain(mapped_value, -512, 512);
            }
        }
    }
    data_changed_flag = true;
}

/**
 * @brief Reads the current count from all encoders connected to ESP2.
 */
void read_hmi_encoders()
{
    for (int i = 0; i < NUM_ENCODERS_ESP2; i++)
    {
        long new_count = encoders[i].getCount();
        if (new_count != hmi_encoder_values[i])
        {
            hmi_encoder_values[i] = new_count;
            data_changed_flag = true;
        }
    }
}

// --- PUBLIC FUNCTIONS: INTERFACE FOR MAIN APP ---

void hmi_init()
{
    pinMode(PIN_LEVEL_SHIFTER_OE, OUTPUT);
    digitalWrite(PIN_LEVEL_SHIFTER_OE, HIGH);
    SPI.begin();
    mcp_buttons.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_BUTTONS);
    mcp_led_rows.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_LED_ROWS);
    mcp_led_cols.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_LED_COLS);

    for (int i = 0; i < 8; i++)
        mcp_buttons.pinMode(i, OUTPUT);
    for (int i = 8; i < 16; i++)
        mcp_buttons.pinMode(i, INPUT_PULLUP);

    for (int i = 0; i < 16; i++)
    {
        mcp_led_rows.pinMode(i, OUTPUT);
        mcp_led_cols.pinMode(i, OUTPUT);
    }

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    for (int i = 0; i < NUM_ENCODERS_ESP2; i++)
    {
        encoders[i].attachFullQuad(ENC2_A_PINS[i], ENC2_B_PINS[i]);
        encoders[i].clearCount();
    }
}

void hmi_task()
{
    update_keypad_states();
    process_joysticks();
    read_hmi_encoders();
    update_led_matrix();
}

bool hmi_data_has_changed()
{
    if (data_changed_flag)
    {
        data_changed_flag = false;
        return true;
    }
    return false;
}

void get_hmi_data(struct_message_from_esp2 *data)
{
    memcpy(data->button_matrix_states, current_button_bitmask, sizeof(data->button_matrix_states));
    memcpy(data->joystick_values, processed_joystick_values, sizeof(data->joystick_values));
    // memcpy(data->hmi_encoder_values, hmi_encoder_values, sizeof(data->hmi_encoder_values)); // Uncomment if added to struct
}

void update_leds_from_lcnc(const struct_message_to_hmi &data)
{
    if (memcmp(current_led_states, data.led_matrix_states, sizeof(current_led_states)) != 0)
    {
        data_changed_flag = true;
    }
    memcpy(current_led_states, data.led_matrix_states, sizeof(current_led_states));
}

void get_live_status_data(uint8_t *btn_buf, uint8_t *led_buf)
{
    memcpy(btn_buf, current_button_bitmask, sizeof(current_button_bitmask));
    memcpy(led_buf, current_led_states, sizeof(current_led_states));
}

void evaluate_action_bindings(const struct_message_to_hmi &lcnc_data)
{
    joystick_1_enabled = true;

    for (int i = 0; i < MAX_ACTION_BINDINGS; i++)
    {
        const auto &binding = web_cfg.bindings[i];
        if (!binding.is_active)
            continue;

        uint16_t status_word_to_check = (binding.trigger < 16)
                                            ? lcnc_data.machine_status
                                            : lcnc_data.spindle_coolant_status;

        uint8_t bit_to_check = binding.trigger % 16;

        if (bitRead(status_word_to_check, bit_to_check))
        {
            switch (binding.action)
            {
            case DISABLE_JOYSTICK_1:
                joystick_1_enabled = false;
                break;
            case ENABLE_JOYSTICK_1:
                break;
            }
        }
    }
}