/**
 * @file config_esp2.h
 * @brief Hardware configuration for ESP2 (The HMI Controller).
 *
 * This file defines static, compile-time settings related to the physical
 * hardware layout of the HMI panel. It specifies which GPIO pins are used
 * for various components and defines the static properties of complex
 * peripherals like joysticks. These are considered the "single source of truth"
 * for the physical hardware build.
 */

#ifndef CONFIG_ESP2_H
#define CONFIG_ESP2_H
#include <Arduino.h>

// --- NETWORK & DEBUGGING ---
#define DEBUG_ENABLED true
#define WIFI_SSID "Your_SSID"
#define WIFI_PASSWORD "Your_Password"
#define OTA_HOSTNAME "linuxcnc-hmi"

// --- GPIO ASSIGNMENT ---
// SPI pins for MCP23S17 I/O Expanders (Standard VSPI).
#define PIN_MCP_MOSI 23
#define PIN_MCP_MISO 19
#define PIN_MCP_SCK 18
#define PIN_MCP_CS 5 // Shared Chip Select pin for all MCPs.

// Control pin for the TXS0108E Level Shifters' Output Enable.
#define PIN_LEVEL_SHIFTER_OE 4

// Potentiometers (must use ADC1 pins as they are more stable).
#define NUM_POTIS 6
const int POTI_PINS[NUM_POTIS] = {36, 39, 32, 33, 35, 34}; // Physical ADC pins.

// --- ROTARY ENCODERS (ESP2) ---
#define NUM_ENCODERS_ESP2 2
const int ENC2_A_PINS[NUM_ENCODERS_ESP2] = {25, 26}; // Example pins
const int ENC2_B_PINS[NUM_ENCODERS_ESP2] = {27, 14}; // Example pins

// --- I/O EXPANDER HARDWARE ADDRESSES ---
// Hardware addresses (A0-A2) set on the MCP23S17 chips.
#define MCP_ADDR_BUTTONS 0
#define MCP_ADDR_LED_ROWS 1
#define MCP_ADDR_LED_COLS 2
#define MCP_ADDR_ROT_SWITCHES 3

// --- MATRIX CONFIGURATION ---
#define MATRIX_ROWS 8
#define MATRIX_COLS 8
#define MAX_BUTTONS (MATRIX_ROWS * MATRIX_COLS)
#define MAX_LEDS (MATRIX_ROWS * MATRIX_COLS)

// --- HMI ELEMENT DEFINITIONS ---

// Defines the source of an LED's state, used in the dynamic web config.
enum LedBinding
{
    UNBOUND,         // LED is controlled independently (e.g., via web UI only)
    BOUND_TO_BUTTON, // LED state mirrors a button's state
    BOUND_TO_LCNC    // LED state is controlled by a bit from LinuxCNC
};

// Defines special actions a button can perform in addition to its normal function.
enum class ButtonExtraAction
{
    NONE,
    TOGGLE_JOYSTICK_AXIS_LOCK
};

/**
 * @brief Defines named constants for button indices.
 * This avoids using "magic numbers" in the code, making configuration
 * more readable and less error-prone. The order here must match the
 * order in the button_configs array.
 */
enum ButtonIndex
{
    BTN_JOYSTICK_1_BUTTON,
    // Add other named buttons here for clear linking...
    MAX_BUTTONS_DEFINED // This automatically provides the total count of defined buttons.
};

/**
 * @brief Defines the static properties of a single button.
 * The functional logic (name, toggle mode) is set in the web UI.
 */
struct ButtonConfig
{
    uint8_t matrix_row;
    uint8_t matrix_col;
    ButtonExtraAction extra_action;
    int target_joystick_index;
    int target_axis_index;
};

// The order of initialization here MUST match the order in the ButtonIndex enum.
const ButtonConfig button_configs[MAX_BUTTONS_DEFINED] = {
    /* BTN_JOYSTICK_1_BUTTON */ {7, 0, ButtonExtraAction::TOGGLE_JOYSTICK_AXIS_LOCK, 0, 0} // CORRECTED: Added scope 'ButtonExtraAction::'
    // ... define matrix positions and actions for other buttons from the enum
};

// --- JOYSTICK STATIC CONFIGURATION ---
#define NUM_JOYSTICK_AXES 3
#define NUM_JOYSTICKS 1 // CORRECTED: Removed trailing 'S'

// Defines the physical potentiometer assignment for a single joystick axis.
// This is a hardware-level setting and is not configurable via the web UI.
struct JoystickAxisStaticConfig
{
    uint8_t poti_index; // The index from the POTI_PINS array.
};

// Defines the entire static hardware mapping for a complete joystick.
struct JoystickStaticConfig
{
    const char *name;
    JoystickAxisStaticConfig axes[NUM_JOYSTICK_AXES];
    ButtonIndex button_index; // Links this joystick to a button defined in the ButtonIndex enum.
};

// Array to configure all joysticks in the system.
const JoystickStaticConfig joystick_configs[NUM_JOYSTICKS] = {
    {
        "Primary Joystick", // Name of the joystick (used in web UI)
        {
            // Axis assignments to potentiometers
            {0}, // X-Axis is connected to POTI_PINS[0]
            {1}, // Y-Axis is connected to POTI_PINS[1]
            {2}  // Z-Axis is connected to POTI_PINS[2]
        },
        BTN_JOYSTICK_1_BUTTON // Links this joystick config to its dedicated button.
    }};

#endif // CONFIG_ESP2_H