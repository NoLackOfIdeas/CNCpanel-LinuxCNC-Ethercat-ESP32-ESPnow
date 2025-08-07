/**
 * @file config_esp3.h
 * @brief Hardware, network, and UI‐config definitions
 *        for the ESP32-S3 Handheld Pendant.
 */

#pragma once

#include <Arduino.h>
#include "shared_structures.h"
#include "ui.h" // ui_set_… prototypes

// bring in the one global PendantWebConfig instance
extern PendantWebConfig pendant_web_cfg;

//------------------------------------------------------------------------------
// NETWORK & DEBUGGING
//------------------------------------------------------------------------------
#define DEBUG_ENABLED 1
#define WIFI_SSID "Your_SSID"
#define WIFI_PASSWORD "Your_Password"
#define OTA_HOSTNAME_PENDANT "linuxcnc-pendant"

//------------------------------------------------------------------------------
// HARDWARE OPTIONS
//------------------------------------------------------------------------------
#define PENDANT_HAS_TOUCHSCREEN 1
#define PENDANT_HAS_BUTTON_MATRIX 1
#define PENDANT_HAS_LEDS 1
#define PENDANT_HAS_FEED_OVERRIDE_ENCODER 1
#define PENDANT_HAS_RAPID_OVERRIDE_ENCODER 1
#define PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER 1

//------------------------------------------------------------------------------
// ADC SELECTORS
//------------------------------------------------------------------------------
#define PIN_AXIS_SELECTOR_ADC A0
#define PIN_STEP_SELECTOR_ADC A1

constexpr int NUM_AXIS_SELECTOR_POSITIONS = 7;
constexpr int NUM_STEP_SELECTOR_POSITIONS = 4;

//------------------------------------------------------------------------------
// HANDWHEEL ENCODER PINS
//------------------------------------------------------------------------------
#define PIN_HW_ENCODER_A 22
#define PIN_HW_ENCODER_B 23

//------------------------------------------------------------------------------
// Physical‐pin definitions in PendantConfig namespace
//------------------------------------------------------------------------------
namespace PendantConfig
{

        // LCD (Arduino_GFX)
        constexpr uint8_t LCD_CS = 10;
        constexpr uint8_t LCD_DC = 9;
        constexpr uint8_t LCD_MOSI = 11;
        constexpr uint8_t LCD_SCK = 12;
        constexpr uint8_t LCD_RST = 8;
        constexpr uint8_t LCD_BL = 7;

#if PENDANT_HAS_TOUCHSCREEN
        constexpr uint8_t TOUCH_CS = 6;
        constexpr uint8_t TOUCH_MISO = 13;
        constexpr uint8_t TOUCH_IRQ = 5;
#endif

#if PENDANT_HAS_BUTTON_MATRIX
        static const uint8_t rowPins[] = {38, 39, 40, 41};
        static const uint8_t colPins[] = {42, 45, 46, 47, 48};
        constexpr size_t NUM_ROWS = sizeof(rowPins) / sizeof(rowPins[0]);
        constexpr size_t NUM_COLS = sizeof(colPins) / sizeof(colPins[0]);
        constexpr size_t MAX_BUTTONS = NUM_ROWS * NUM_COLS;
#endif

#if PENDANT_HAS_LEDS
        static const uint8_t ledPins[] = {15, 16, 17};
        constexpr size_t NUM_LEDS = sizeof(ledPins) / sizeof(ledPins[0]);
#endif

#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
        constexpr uint8_t FEED_OVR_A = 18;
        constexpr uint8_t FEED_OVR_B = 21;
#endif

#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
        constexpr uint8_t RAPID_OVR_A = 33;
        constexpr uint8_t RAPID_OVR_B = 34;
#endif

#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
        constexpr uint8_t SPINDLE_OVR_A = 35;
        constexpr uint8_t SPINDLE_OVR_B = 36;
#endif

        // alias for any code still doing PendantConfig::current
        static PendantWebConfig &current = pendant_web_cfg;
}

//------------------------------------------------------------------------------
// Legacy‐style macros
//------------------------------------------------------------------------------
#define PIN_LCD_CS PendantConfig::LCD_CS
#define PIN_LCD_DC PendantConfig::LCD_DC
#define PIN_LCD_MOSI PendantConfig::LCD_MOSI
#define PIN_LCD_SCK PendantConfig::LCD_SCK
#define PIN_LCD_RST PendantConfig::LCD_RST
#define PIN_LCD_BL PendantConfig::LCD_BL

#define PENDANT_MATRIX_ROWS PendantConfig::NUM_ROWS
#define PENDANT_MATRIX_COLS PendantConfig::NUM_COLS
#define MAX_BUTTONS_DEFINED PendantConfig::MAX_BUTTONS
#define PENDANT_ROW_PINS PendantConfig::rowPins
#define PENDANT_COL_PINS PendantConfig::colPins

#define PENDANT_LED_PINS PendantConfig::ledPins

// Provide a plain constexpr for legacy code
constexpr int NUM_PENDANT_LEDS = static_cast<int>(PendantConfig::NUM_LEDS);

#define PIN_FEED_OVR_A PendantConfig::FEED_OVR_A
#define PIN_FEED_OVR_B PendantConfig::FEED_OVR_B
#define PIN_RAPID_OVR_A PendantConfig::RAPID_OVR_A
#define PIN_RAPID_OVR_B PendantConfig::RAPID_OVR_B
#define PIN_SPINDLE_OVR_A PendantConfig::SPINDLE_OVR_A
#define PIN_SPINDLE_OVR_B PendantConfig::SPINDLE_OVR_B
