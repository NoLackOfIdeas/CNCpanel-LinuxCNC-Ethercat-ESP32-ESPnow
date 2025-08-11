/**
 * @file config_esp3.h
 * @brief Hardware pinout and feature configuration for the ESP32-S3 Pendant.
 *
 * This file centralizes all GPIO pin assignments and hardware feature flags.
 */

#pragma once

#include <Arduino.h> // Provides core types like uint8_t and pin names (A0)

// --- Top-Level Hardware Features ---
constexpr bool PENDANT_HAS_TOUCHSCREEN = true;
constexpr bool PENDANT_HAS_BUTTON_MATRIX = true;
constexpr bool PENDANT_HAS_LEDS = true;
constexpr bool PENDANT_HAS_FEED_OVERRIDE_ENCODER = true;
constexpr bool PENDANT_HAS_RAPID_OVERRIDE_ENCODER = true;
constexpr bool PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER = true;

// --- Pin Definitions Namespace ---
namespace Pinout
{
        // Network Configuration
        constexpr const char *WIFI_SSID = "Your_SSID";
        constexpr const char *WIFI_PASSWORD = "Your_Password";
        constexpr const char *OTA_HOSTNAME = "linuxcnc-pendant";

        // ADC Pins for analog selectors
        constexpr uint8_t AXIS_SELECTOR_ADC = A0;
        constexpr uint8_t STEP_SELECTOR_ADC = A1;

        // Main Handwheel Encoder
        constexpr uint8_t HW_ENCODER_A = 22;
        constexpr uint8_t HW_ENCODER_B = 23;

// Button Matrix (if enabled)
#if PENDANT_HAS_BUTTON_MATRIX
        constexpr uint8_t PENDANT_ROW_PINS[] = {38, 39, 40, 41};
        constexpr uint8_t PENDANT_COL_PINS[] = {42, 45, 46, 47, 48};
        constexpr size_t PENDANT_MATRIX_ROWS = sizeof(PENDANT_ROW_PINS) / sizeof(PENDANT_ROW_PINS[0]);
        constexpr size_t PENDANT_MATRIX_COLS = sizeof(PENDANT_COL_PINS) / sizeof(PENDANT_COL_PINS[0]);
#endif

// Status LEDs (if enabled)
#if PENDANT_HAS_LEDS
        constexpr uint8_t PENDANT_LED_PINS[] = {15, 16, 17};
        constexpr size_t NUM_PENDANT_LEDS = sizeof(PENDANT_LED_PINS) / sizeof(PENDANT_LED_PINS[0]);
#endif

// --- Override Encoders (if enabled) ---
#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
        constexpr uint8_t PIN_FEED_OVR_A = 18;
        constexpr uint8_t PIN_FEED_OVR_B = 21;
#endif

#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
        constexpr uint8_t PIN_RAPID_OVR_A = 33;
        constexpr uint8_t PIN_RAPID_OVR_B = 34;
#endif

#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
        constexpr uint8_t PIN_SPINDLE_OVR_A = 35;
        constexpr uint8_t PIN_SPINDLE_OVR_B = 36;
#endif
}

// --- Display & Touch Pins (separate for LGFX_Config.h) ---
namespace DisplayConfig
{
        constexpr uint8_t PIN_LCD_SCLK = 12;
        constexpr uint8_t PIN_LCD_MOSI = 11;
        constexpr uint8_t PIN_LCD_MISO = 13;
        constexpr uint8_t PIN_LCD_CS = 10;
        constexpr uint8_t PIN_LCD_DC = 9;
        constexpr uint8_t PIN_LCD_RST = 8;
        constexpr uint8_t PIN_LCD_BL = 7;
        constexpr uint8_t PIN_TOUCH_SDA = 18;
        constexpr uint8_t PIN_TOUCH_SCL = 19;
        constexpr uint8_t PIN_TOUCH_INT = 5;
}
