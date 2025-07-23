#ifndef CONFIG_ESP2_H
#define CONFIG_ESP2_H

#include <Arduino.h>
#include "shared_structures.h" // Für MAX_... Definitionen

// =================================================================
// DEBUGGING & NETWORK
// =================================================================
#define DEBUG_ENABLED true
#define WIFI_SSID "Your_SSID"
#define WIFI_PASSWORD "Your_Password"
#define OTA_HOSTNAME "linuxcnc-hmi"

// =================================================================
// GPIO ASSIGNMENT ESP2
// =================================================================
// SPI for MCP23S17 Expanders (Standard VSPI)
#define PIN_MCP_MOSI 23
#define PIN_MCP_MISO 19
#define PIN_MCP_SCK 18
#define PIN_MCP_CS 5 // Shared CS Pin

// Control for TXS0108E Level Shifters
#define PIN_LEVEL_SHIFTER_OE 4

// Potentiometers (use ADC1 pins only)
const int POTI_PINS = {36, 39, 32, 33, 35, 34};

// Rotary Encoders
const int ENC2_A_PINS = {25, 26};
const int ENC2_B_PINS = {27, 14};
#define ENC2_GLITCH_FILTER 1023

// =================================================================
// MCP23S17 CONFIGURATION
// =================================================================
#define NUM_MCP_DEVICES 4
#define MCP_ADDR_BUTTONS 0
#define MCP_ADDR_LED_ROWS 1
#define MCP_ADDR_LED_COLS 2
#define MCP_ADDR_ROT_SWITCHES 3

// =================================================================
// BUTTON & LED MATRIX CONFIGURATION (STATIC PART)
// =================================================================
#define MATRIX_ROWS 8
#define MATRIX_COLS 8
#define MAX_BUTTONS (MATRIX_ROWS * MATRIX_COLS)
#define MAX_LEDS (MATRIX_ROWS * MATRIX_COLS)

const uint8_t BUTTON_ROW_PINS = {0, 1, 2, 3, 4, 5, 6, 7};       // GPA
const uint8_t BUTTON_COL_PINS = {8, 9, 10, 11, 12, 13, 14, 15}; // GPB

const uint8_t LED_ROW_PINS = {0, 1, 2, 3, 4, 5, 6, 7};
const uint8_t LED_COL_PINS = {0, 1, 2, 3, 4, 5, 6, 7};

// =================================================================
// LOGICAL BUTTON & LED CONFIGURATION
// =================================================================
struct ButtonConfig
{
    const char *name;
    bool is_toggle;
    int radio_group_id;
};

const ButtonConfig button_configs = {
    // Name, Toggle?, Radiogruppe
    {"Cycle Start", false, 0},
    {"Feed Hold", false, 0},
    {"Stop", false, 0},
    {"Unused", false, 0},
    {"Unused", false, 0},
    {"Unused", false, 0},
    {"Unused", false, 0},
    {"Unused", false, 0},
    {"Mode AUTO", true, 1},
    {"Mode MDI", true, 1},
    {"Mode JOG", true, 1},
    {"Unused", false, 0},
    {"Unused", false, 0},
    {"Unused", false, 0},
    {"Unused", false, 0},
    {"Unused", false, 0},
    //... weitere 48 Tasten definieren
};

enum LedBinding
{
    UNBOUND,
    BOUND_TO_BUTTON,
    BOUND_TO_LCNC
};

struct LedConfig
{
    const char *name;
    LedBinding binding_type;
    int bound_button_index;
    int lcnc_state_bit;
};

const LedConfig led_configs = {
    // Name, Bindungstyp, Gekoppelte Taste, LCNC-Bit
    {"Cycle Active", BOUND_TO_LCNC, -1, 0},
    {"Feed Hold Active", BOUND_TO_LCNC, -1, 1},
    {"Stop Active", BOUND_TO_LCNC, -1, 2},
    {"Unused", UNBOUND, -1, -1},
    {"Unused", UNBOUND, -1, -1},
    {"Unused", UNBOUND, -1, -1},
    {"Unused", UNBOUND, -1, -1},
    {"Unused", UNBOUND, -1, -1},
    {"AUTO Mode LED", BOUND_TO_BUTTON, 8, -1},
    {"MDI Mode LED", BOUND_TO_BUTTON, 9, -1},
    {"JOG Mode LED", BOUND_TO_BUTTON, 10, -1},
    {"Unused", UNBOUND, -1, -1},
    {"Unused", UNBOUND, -1, -1},
    {"Unused", UNBOUND, -1, -1},
    {"Unused", UNBOUND, -1, -1},
    {"Unused", UNBOUND, -1, -1},
    //... weitere 48 LEDs definieren
};

#endif // CONFIG_ESP2_H