/**
 * @file hmi_handler.cpp
 * @brief Implements the core logic for managing all HMI peripherals.
 *
 * This file contains the state machine for debouncing the button matrix,
 * the processing pipeline for the analog joysticks, and the multiplexing
 * logic for driving the physical LED matrix. It abstracts all low-level
 * hardware interaction for the main application.
 *
 * Concepts are derived from the project document "Integriertes Dual-ESP32-Steuerungssystem".
 */

#include "hmi_handler.h"
#include "config_esp2.h"
#include "persistence.h"
#include <Adafruit_MCP23X17.h>
#include <SPI.h>

// --- CONFIGURATION & CONSTANTS ---
// Time in milliseconds a button must be stable to register a state change.
const unsigned long KEY_DEBOUNCE_MS = 50;

// --- INTERNAL DATA STRUCTURES ---
// Defines the possible states for the key debouncing state machine.
enum class KeyState
{
    IDLE,
    PRESSED,
    HELD,
    RELEASED
};

// Holds all state information for a single key in the matrix.
struct KeyInfo
{
    KeyState state = KeyState::IDLE;
};

// --- GLOBAL OBJECTS AND STATE VARIABLES ---
// MCP23S17 I/O expander objects.
Adafruit_MCP23X17 mcp_buttons;
Adafruit_MCP23X17 mcp_led_rows;
Adafruit_MCP23X17 mcp_led_cols;

// External reference to the dynamic configuration loaded from NVS.
extern WebConfig web_cfg;

// State tracking arrays.
static KeyInfo key_matrix[MATRIX_ROWS][MATRIX_COLS];
static uint8_t current_button_bitmask[MATRIX_ROWS] = {0};
static uint8_t current_led_states[MATRIX_ROWS] = {0}; // Local buffer for desired LED states.
static int16_t processed_joystick_values[NUM_JOYSTICKS][NUM_JOYSTICK_AXES];
static bool joystick_axis_locked[NUM_JOYSTICKS][NUM_JOYSTICK_AXES] = {false};
static bool data_changed_flag = false;
static uint8_t current_led_scan_col = 0; // Tracks the current column for LED multiplexing.

// --- PRIVATE FUNCTIONS: CORE LOGIC ---

/**
 * @brief Drives the physical LED matrix using multiplexing.
 * This function should be called very rapidly in the main loop to create a
 * stable, flicker-free image. It drives one column per call to remain non-blocking.
 */
void update_led_matrix()
{
    // 1. Turn off all column drivers to prevent ghosting from the previous column.
    // We assume N-channel MOSFETs for low-side switching, so writing LOW activates the column.
    mcp_led_cols.writeGPIOAB(0x0000);

    // 2. Prepare the data for the ROW pins for the CURRENT column to be displayed.
    uint16_t row_data = 0;
    for (int i = 0; i < MATRIX_ROWS; i++)
    {
        // Check if the LED at (row i, current column) should be on.
        if (bitRead(current_led_states[i], current_led_scan_col))
        {
            bitSet(row_data, i); // Set the bit for the i-th row pin.
        }
    }
    mcp_led_rows.writeGPIOAB(row_data);

    // 3. Turn on the driver for the current column.
    mcp_led_cols.digitalWrite(current_led_scan_col, HIGH);

    // 4. Advance to the next column for the next call, wrapping around at the end.
    current_led_scan_col = (current_led_scan_col + 1) % MATRIX_COLS;
}

/**
 * @brief Scans the button matrix and updates the state of each key using a
 * debouncing state machine. Also handles special button actions.
 */
void update_keypad_states()
{
    bool state_has_changed = false;

    // Iterate through each row of the matrix
    for (int row = 0; row < MATRIX_ROWS; row++)
    {
        mcp_buttons.digitalWrite(row, LOW); // Activate the current row by pulling it low

        // Scan all columns for the active row
        for (int col = 0; col < MATRIX_COLS; col++)
        {
            int button_idx = row * MATRIX_COLS + col;
            if (button_idx >= MAX_BUTTONS_DEFINED)
                continue; // Skip unconfigured buttons

            bool is_pressed = (mcp_buttons.digitalRead(col + 8) == LOW);
            KeyInfo &key = key_matrix[row][col];
            KeyState old_state = key.state;

            // Debouncing state machine for the current key
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

            // If a state change occurred, update bitmasks and handle actions
            if (key.state != old_state)
            {
                state_has_changed = true;
                if (key.state == KeyState::PRESSED)
                {
                    bitSet(current_button_bitmask[row], col);
                    // Handle extra actions defined in config.h
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
        mcp_buttons.digitalWrite(row, HIGH); // Deactivate the current row
    }

    if (state_has_changed)
        data_changed_flag = true;
}

/**
 * @brief Reads all potentiometers and processes them according to the combined
 * static (pin mapping) and dynamic (tuning) joystick configuration.
 */
void process_joysticks()
{
    for (int i = 0; i < NUM_JOYSTICKS; i++)
    {
        for (int j = 0; j < NUM_JOYSTICK_AXES; j++)
        {
            // If the axis is locked by a button press, set its value to 0.
            if (joystick_axis_locked[i][j])
            {
                processed_joystick_values[i][j] = 0;
                continue;
            }

            // Get the configuration from both static and dynamic sources
            const auto &static_cfg = joystick_configs[i].axes[j];
            const auto &dynamic_cfg = web_cfg.joysticks[i][j];

            // 1. Read raw ADC value from the statically assigned pin
            int raw_value = analogRead(POTI_PINS[static_cfg.poti_index]);

            // 2. Apply deadzone from the dynamic configuration
            int center_value = 2048; // Midpoint of a 12-bit ADC (0-4095)
            if (abs(raw_value - center_value) < dynamic_cfg.center_deadzone)
            {
                processed_joystick_values[i][j] = 0;
                continue;
            }

            // 3. Map the value from the raw ADC range to the target range (-512 to +512)
            long mapped_value = (raw_value > center_value)
                                    ? map(raw_value, center_value + dynamic_cfg.center_deadzone, 4095, 0, 512)
                                    : map(raw_value, 0, center_value - dynamic_cfg.center_deadzone, -512, 0);

            // 4. Apply sensitivity from the dynamic config
            mapped_value *= dynamic_cfg.sensitivity;

            // 5. Check for inversion from the dynamic config
            if (dynamic_cfg.is_inverted)
            {
                mapped_value *= -1;
            }

            // 6. Constrain the value to the final range and store it
            processed_joystick_values[i][j] = constrain(mapped_value, -512, 512);
        }
    }
    data_changed_flag = true; // Assume joysticks are always moving, so always flag data as changed.
}

// --- PUBLIC FUNCTIONS: INTERFACE FOR MAIN APP ---

void hmi_init()
{
    // Enable level shifters after the ESP32 has booted to prevent issues.
    pinMode(PIN_LEVEL_SHIFTER_OE, OUTPUT);
    digitalWrite(PIN_LEVEL_SHIFTER_OE, HIGH);

    SPI.begin();

    // Initialize all MCP23S17 I/O expanders with their hardware addresses.
    mcp_buttons.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_BUTTONS);
    mcp_led_rows.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_LED_ROWS);
    mcp_led_cols.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_LED_COLS);

    // Configure MCP pins for the 8x8 button matrix
    for (int i = 0; i < 8; i++)
        mcp_buttons.pinMode(i, OUTPUT); // Rows are outputs
    for (int i = 8; i < 16; i++)
        mcp_buttons.pinMode(i, INPUT_PULLUP); // Columns are inputs with pullups

    // Configure MCP pins for the LED matrix (all pins are outputs)
    for (int i = 0; i < 16; i++)
    {
        mcp_led_rows.pinMode(i, OUTPUT);
        mcp_led_cols.pinMode(i, OUTPUT);
    }
}

void hmi_task()
{
    // Call the core logic functions repeatedly.
    update_keypad_states();
    process_joysticks();
    update_led_matrix(); // Drive the LEDs
}

bool hmi_data_has_changed()
{
    if (data_changed_flag)
    {
        data_changed_flag = false; // Reset flag after it has been queried
        return true;
    }
    return false;
}

void get_hmi_data(struct_message_to_esp1 *data)
{
    // Copy all processed input data into the data structure for ESP-NOW transmission.
    memcpy(data->button_matrix_states, current_button_bitmask, sizeof(current_button_bitmask));
    memcpy(data->joystick_values, processed_joystick_values, sizeof(processed_joystick_values));
}

void update_leds_from_lcnc(const struct_message_to_esp2 &data)
{
    // If the new LED state from LinuxCNC is different from the current one,
    // flag that data has changed so the web UI gets updated.
    if (memcmp(current_led_states, data.led_matrix_states, sizeof(current_led_states)) != 0)
    {
        data_changed_flag = true;
    }
    // Copy the new LED state into the local buffer. The update_led_matrix()
    // function will then display it on the physical LEDs.
    memcpy(current_led_states, data.led_matrix_states, sizeof(current_led_states));
}

void get_live_status_data(uint8_t *btn_buf, uint8_t *led_buf)
{
    // Provides the main application with the current live state for WebSocket broadcasting.
    memcpy(btn_buf, current_button_bitmask, sizeof(current_button_bitmask));
    memcpy(led_buf, current_led_states, sizeof(current_led_states));
}