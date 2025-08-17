#pragma once
#pragma message(">> Including config_esp3.h from: " __FILE__)
#include <Arduino.h>

// Feature flags must be macros for #if checks
#define PENDANT_HAS_TOUCHSCREEN 1   // this always has to be 1
#define PENDANT_HAS_BUTTON_MATRIX 1 //
#define PENDANT_HAS_LEDS 1          //
#define PENDANT_HAS_FEED_OVERRIDE_ENCODER 1
#define PENDANT_HAS_RAPID_OVERRIDE_ENCODER 0   // set to 0 if BUTTON_MATRIX > 2x2
#define PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER 0 //
#define PENDANT_HAS_HANDWEEL_ENCODER 1

namespace Pinout
{
        // Wi-Fi + OTA (no change)
        constexpr const char *WIFI_SSID = "WummiWLAN5Ghz";
        constexpr const char *WIFI_PASSWORD = "Wusch1225";
        constexpr const char *OTA_HOSTNAME = "linuxcnc-pendant";

        // Analog selectors (ADC1 channels - corrected for ESP32-S3)
        constexpr uint8_t AXIS_SELECTOR_ADC = 1; // GPIO1 (ADC1_CH0)
        constexpr uint8_t STEP_SELECTOR_ADC = 2; // GPIO2 (ADC1_CH1)

        // Handwheel encoder (corrected for available pins)
        constexpr uint8_t HW_ENCODER_A = 15;
        constexpr uint8_t HW_ENCODER_B = 16;

        // Button matrix - 2x2 matrix (4 buttons total)
#if PENDANT_HAS_BUTTON_MATRIX
        constexpr uint8_t PENDANT_ROW_PINS[] = {17, 18}; // 2 rows
        constexpr uint8_t PENDANT_COL_PINS[] = {3, 21};  // 2 columns (avoiding GPIO4/5 conflicts)
        constexpr size_t PENDANT_MATRIX_ROWS = sizeof(PENDANT_ROW_PINS) / sizeof(PENDANT_ROW_PINS[0]);
        constexpr size_t PENDANT_MATRIX_COLS = sizeof(PENDANT_COL_PINS) / sizeof(PENDANT_COL_PINS[0]);
#endif

        // Status LEDs - disabled
#if PENDANT_HAS_LEDS
        constexpr uint8_t PENDANT_LED_PINS[] = {};
        constexpr size_t NUM_PENDANT_LEDS = sizeof(PENDANT_LED_PINS) / sizeof(PENDANT_LED_PINS[0]);
#endif

        // Feed-override encoder
#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
        constexpr uint8_t PIN_FEED_OVR_A = 14; // Now using freed GPIO14
        constexpr uint8_t PIN_FEED_OVR_B = 13;
#endif

        // Rapid-override encoder - DISABLED to save pins
#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
        constexpr uint8_t PIN_RAPID_OVR_A = 18;
        constexpr uint8_t PIN_RAPID_OVR_B = 19;
#endif

        // Spindle-override encoder - DISABLED to save pins
#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
        constexpr uint8_t PIN_SPINDLE_OVR_A = 20;
        constexpr uint8_t PIN_SPINDLE_OVR_B = 21;
#endif
}

namespace DisplayConfig
{
        // LCD SPI (VSPI) - no conflicts now
        constexpr uint8_t PIN_LCD_SCLK = 12;
        constexpr uint8_t PIN_LCD_MOSI = 11;
        constexpr uint8_t PIN_LCD_MISO = 19; // Not used, but defined for completeness
        constexpr uint8_t PIN_LCD_CS = 10;
        constexpr uint8_t PIN_LCD_DC = 5;  // No longer conflicts
        constexpr uint8_t PIN_LCD_RST = 4; // No longer conflicts
        constexpr uint8_t PIN_LCD_BL = 7;

        // Touch I²C
        constexpr uint8_t PIN_TOUCH_SDA = 8;
        constexpr uint8_t PIN_TOUCH_SCL = 9;
        constexpr uint8_t PIN_TOUCH_INT = 6;
}

/*
MY MAC-Address: 50:78:7D:16:9C:EC

PIN USAGE SUMMARY for ESP32-S3-N16R8 (44-pin):
Available GPIOs: 1-18, 21 (19-20 reserved for USB)

GPIO1  - Axis selector ADC
GPIO2  - Step selector ADC
GPIO3  - Button matrix col 0
GPIO4  - LCD RST (no longer conflicts)
GPIO5  - LCD DC (no longer conflicts)
GPIO6  - Touch interrupt
GPIO7  - LCD backlight
GPIO8  - Touch SDA
GPIO9  - Touch SCL
GPIO10 - LCD CS
GPIO11 - LCD MOSI
GPIO12 - LCD SCLK
GPIO13 - Feed encoder B (MISO not used)
GPIO14 - Feed encoder A
GPIO15 - Handwheel encoder A
GPIO16 - Handwheel encoder B
GPIO17 - Button matrix row 0
GPIO18 - Button matrix row 1
GPIO21 - Button matrix col 1

✅ If 2x2 Button matrix should be increased: either set PENDANT_HAS_RAPID_OVERRIDE_ENCODER 0 OR USE MCP23S17 I/O Expanders or similar.

ESP32-S3-N16R8 44-pin package typically has:
GPIO1-21 (but 19-20 are USB / used for JTAG-Debugging)
GPIO26-48 are NOT available in 44-pin package
GPIO26-37 are used internally for Flash/PSRAM

Problematic pins:
GPIO34, 35 - Don't exist on ESP32-S3
GPIO25-48 - Most don't exist on 44-pin package
GPIO26-37 - Used for internal Flash/PSRAM

Available GPIOs on 44-pin board:
GPIO1-18 (avoid 19-20 if using USB)
GPIO21

That's it! Only about 18-19 usable pins total.

*/