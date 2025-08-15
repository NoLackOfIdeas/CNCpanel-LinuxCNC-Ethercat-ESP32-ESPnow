#pragma once
#pragma message(">> Including config_esp3.h from: " __FILE__)
#include <Arduino.h>

// Feature flags must be macros for #if checks
#define PENDANT_HAS_TOUCHSCREEN 1 // this always has to be 1
#define PENDANT_HAS_BUTTON_MATRIX 0
#define PENDANT_HAS_LEDS 0
#define PENDANT_HAS_FEED_OVERRIDE_ENCODER 0
#define PENDANT_HAS_RAPID_OVERRIDE_ENCODER 0
#define PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER 0
#define PENDANT_HAS_HANDWEEL_ENCODER 0

namespace Pinout
{
        // Wi-Fi + OTA (no change)
        constexpr const char *WIFI_SSID = "WummiWLAN5Ghz";
        constexpr const char *WIFI_PASSWORD = "Wusch1225";
        constexpr const char *OTA_HOSTNAME = "linuxcnc-pendant";

        // Analog selectors (ADC1 channels on GPIO34,35)
        constexpr uint8_t AXIS_SELECTOR_ADC = 34;
        constexpr uint8_t STEP_SELECTOR_ADC = 35;

        // Handwheel encoder
        constexpr uint8_t HW_ENCODER_A = 22;
        constexpr uint8_t HW_ENCODER_B = 23;

        // Button matrix rows and columns
#if PENDANT_HAS_BUTTON_MATRIX
        constexpr uint8_t PENDANT_ROW_PINS[] = {25, 26, 27, 28};
        constexpr uint8_t PENDANT_COL_PINS[] = {40, 41, 42, 43, 44};
        constexpr size_t PENDANT_MATRIX_ROWS = sizeof(PENDANT_ROW_PINS) / sizeof(PENDANT_ROW_PINS[0]);
        constexpr size_t PENDANT_MATRIX_COLS = sizeof(PENDANT_COL_PINS) / sizeof(PENDANT_COL_PINS[0]);
#endif

        // Status LEDs
#if PENDANT_HAS_LEDS
        constexpr uint8_t PENDANT_LED_PINS[] = {29, 30, 31};
        constexpr size_t NUM_PENDANT_LEDS = sizeof(PENDANT_LED_PINS) / sizeof(PENDANT_LED_PINS[0]);
#endif

        // Feed-override encoder
#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
        constexpr uint8_t PIN_FEED_OVR_A = 16;
        constexpr uint8_t PIN_FEED_OVR_B = 17;
#endif

        // Rapid-override encoder
#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
        constexpr uint8_t PIN_RAPID_OVR_A = 18;
        constexpr uint8_t PIN_RAPID_OVR_B = 19;
#endif

        // Spindle-override encoder
#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
        constexpr uint8_t PIN_SPINDLE_OVR_A = 20;
        constexpr uint8_t PIN_SPINDLE_OVR_B = 21;
#endif
}

namespace DisplayConfig
{
        // LCD SPI (VSPI)
        constexpr uint8_t PIN_LCD_SCLK = 12;
        constexpr uint8_t PIN_LCD_MOSI = 11;
        constexpr uint8_t PIN_LCD_MISO = 13; // Not used, but required by SPI
        constexpr uint8_t PIN_LCD_CS = 10;
        constexpr uint8_t PIN_LCD_DC = 5;
        constexpr uint8_t PIN_LCD_RST = 4;
        constexpr uint8_t PIN_LCD_BL = 7;

        // Touch I²C
        constexpr uint8_t PIN_TOUCH_SDA = 8;
        constexpr uint8_t PIN_TOUCH_SCL = 9;
        constexpr uint8_t PIN_TOUCH_INT = 6;
}
