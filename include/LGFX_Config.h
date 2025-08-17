/**
 * @file LGFX_Config.h
 * @brief Custom configuration for LovyanGFX on the ESP32-S3 Pendant.
 */

#pragma once

// Define LGFX_USE_V1 to use the new V1 configuration style
#define LGFX_USE_V1

// IMPORTANT: Define the LVGL color depth for LovyanGFX to prevent type conflicts.
#define LGFX_USE_LVGL_COLOR_DEPTH 16

#include <LovyanGFX.hpp>
#include "esp3/config_esp3.h"
using namespace DisplayConfig;

class LGFX : public lgfx::LGFX_Device
{
    // --- Member Variable Declarations ---
    // You must declare the bus, panel, and touch instances you will use.
    lgfx::Bus_SPI _bus_instance;
    lgfx::Panel_ST7796 _panel_instance;
    lgfx::Touch_FT5x06 _touch_instance; // The FT5x06 driver is compatible with your FT6336U

public:
    LGFX(void)
    {
        // Bus Control Configuration (SPI)
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI3_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.pin_sclk = DisplayConfig::PIN_LCD_SCLK;
            cfg.pin_mosi = DisplayConfig::PIN_LCD_MOSI;
            cfg.pin_miso = -1; // only for headless debugging, set back to DisplayConfig::PIN_LCD_MISO;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Display Panel Control Configuration (ST7796)
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = DisplayConfig::PIN_LCD_CS;
            cfg.pin_rst = DisplayConfig::PIN_LCD_RST;
            cfg.pin_busy = -1;
            cfg.panel_width = 480;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false; // -->just for debugging. Set back to true
            cfg.invert = true;
            cfg.rgb_order = true;
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;
            _panel_instance.config(cfg);
        }

        // Touch Screen Control Configuration (FT6336U)
        {
            auto cfg = _touch_instance.config();
            cfg.x_min = 0;
            cfg.y_min = 0;
            cfg.x_max = 479;
            cfg.y_max = 319;
            cfg.bus_shared = true;
            cfg.offset_rotation = 0;
            cfg.pin_int = DisplayConfig::PIN_TOUCH_INT;
            cfg.pin_sda = DisplayConfig::PIN_TOUCH_SDA;
            cfg.pin_scl = DisplayConfig::PIN_TOUCH_SCL;
            cfg.i2c_port = 0;
            cfg.i2c_addr = 0x38;
            cfg.freq = 400000;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }
};
