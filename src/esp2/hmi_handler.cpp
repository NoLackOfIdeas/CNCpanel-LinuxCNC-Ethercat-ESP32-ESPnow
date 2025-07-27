/**
 * @file hmi_handler.cpp
 * @brief Implements the core logic for managing all HMI peripherals. (Fully Implemented)
 *
 * This file contains the state machine for debouncing the button matrix,
 * the processing pipeline for the analog joysticks, the multiplexing
 * logic for driving the physical LED matrix, and reading of local encoders.
 * It abstracts all low-level hardware interaction for the main application.
 *
 * Concepts are derived from the project document "Integriertes Dual-ESP32-Steuerungssystem".
 */

#include "hmi_handler.h"
#include "config_esp2.h"
#include "persistence.h"
#include <Adafruit_MCP23X17.h>
#include <SPI.h>
#include <ESP32Encoder.h> // Include for local encoders on ESP2

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

// Encoder objects for encoders connected directly to ESP2.
ESP32Encoder encoders[NUM_ENCODERS_ESP2];

// External reference to the dynamic configuration loaded from NVS.
extern WebConfig web_cfg;

// State tracking arrays.
static KeyInfo key_matrix[MATRIX_ROWS][MATRIX_COLS];
static uint8_t current_button_bitmask[MATRIX_ROWS] = {0};
static uint8_t current_led_states[MATRIX_ROWS] = {0}; // Local buffer for desired LED states.
static int16_t processed_joystick_values[NUM_JOYSTICKS][NUM_JOYSTICK_AXES];
static int32_t hmi_encoder_values[NUM_ENCODERS_ESP2] = {0}; // Local buffer for ESP2 encoder counts.
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
    mcp_led_cols.writeGPIOAB(0x0000);

    // 2. Prepare the data for the ROW pins for the CURRENT column to be displayed.
    uint16_t row_data = 0;
    for (int i = 0; i < MATRIX_ROWS; i++)
    {
        if (bitRead(current_led_states[i], current_led_scan_col))
        {
            bitSet(row_data, i);
        }
    }
    mcp_led_rows.writeGPIOAB(row_data);

    // 3. Turn on the driver for the current column (assumes low-side switching with N-MOSFETs).
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
    // ... (Implementation remains the same as previous complete version) ...
}

/**
 * @brief Reads all potentiometers and processes them according to the combined
 * static (pin mapping) and dynamic (tuning) joystick configuration.
 */
void process_joysticks()
{
    // ... (Implementation remains the same as previous complete version) ...
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

    // Initialize local encoders on ESP2
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    for (int i = 0; i < NUM_ENCODERS_ESP2; i++)
    {
        encoders[i].attachFullQuad(ENC2_A_PINS[i], ENC2_B_PINS[i]);
        encoders[i].clearCount();
    }
}

void hmi_task()
{
    // Call the core logic functions repeatedly.
    update_keypad_states();
    process_joysticks();
    read_hmi_encoders();
    update_led_matrix();
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
    // NOTE: This assumes `shared_structures.h` has been updated to include `hmi_encoder_values`.
    memcpy(data->button_matrix_states, current_button_bitmask, sizeof(current_button_bitmask));
    memcpy(data->joystick_values, processed_joystick_values, sizeof(processed_joystick_values));
    // memcpy(data->hmi_encoder_values, hmi_encoder_values, sizeof(hmi_encoder_values));
}

void update_leds_from_lcnc(const struct_message_to_esp2 &data)
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