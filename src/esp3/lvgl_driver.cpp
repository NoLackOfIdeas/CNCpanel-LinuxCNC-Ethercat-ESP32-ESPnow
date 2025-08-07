/**
 * @file lvgl_driver.cpp
 * @brief LVGL v9 display & encoder driver using Arduino_GFX on ESP32-S3.
 *        Implements double-buffered drawing and a 1 ms tick via esp_timer.
 */

#include "lv_conf.h"
#include "lvgl_driver.h"
#include "config_esp3.h" // Defines PendantConfig::LCD_* and get_handwheel_diff()
#include <Arduino_GFX.h>
#include <DataBus/Arduino_ESP32SPI.h>
#include <Display/Arduino_ST7796.h>
#include <Arduino.h>
#include <esp_timer.h> // For esp_timer_create / start_periodic
#include <lvgl.h>      // Core LVGL types & init

// Forward-declare the C function if not in config_esp3.h
extern "C" int get_handwheel_diff();

lv_group_t *g_input_group = nullptr;          // drop ‘static’
esp_timer_handle_t lvgl_tick_timer = nullptr; // define the extern

// Screen resolution and buffer height (lines)
static constexpr int SCREEN_W = 480;
static constexpr int SCREEN_H = 320;
static constexpr int LINES_BUF = 10; // draw 10 lines per buffer

// Two draw buffers for double-buffering (pixel count, not bytes)
static lv_color_t buf1[SCREEN_W * LINES_BUF];
static lv_color_t buf2[SCREEN_W * LINES_BUF];

// LVGL display and input device handles
static lv_display_t *disp = nullptr;
static lv_indev_t *indev = nullptr;
static bool driver_ready = false;

// FPS tracking
static uint32_t flush_count = 0, last_ms = 0;
static float fps_value = 0.0f;

// Arduino_GFX bus + panel instance
static Arduino_ESP32SPI bus(PendantConfig::LCD_DC,
                            PendantConfig::LCD_CS,
                            PendantConfig::LCD_SCK,
                            PendantConfig::LCD_MOSI,
                            /*dc=*/-1);
static Arduino_ST7796 tft(&bus,
                          PendantConfig::LCD_RST,
                          /*rotation=*/1,
                          /*ips=*/true);

// Global LVGL encoder-navigation group

//-----------------------------------------------------------------------------
// LVGL tick callback: called every 1 ms by esp_timer
//-----------------------------------------------------------------------------
static void lv_tick_task(void *)
{
    lv_tick_inc(1);
}

//-----------------------------------------------------------------------------
// Flush callback for LVGL v9: pushes `area` from `color_buf` to the TFT.
//-----------------------------------------------------------------------------
void my_disp_flush(lv_display_t *display,
                   const lv_area_t *area,
                   uint8_t *color_buf)
{
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels(reinterpret_cast<uint16_t *>(color_buf), w * h);
    tft.endWrite();

    lv_display_flush_ready(display);
    ++flush_count;
}

//-----------------------------------------------------------------------------
// Encoder read callback for LVGL v9.
//-----------------------------------------------------------------------------
void encoder_read_cb(lv_indev_t *driver, lv_indev_data_t *data)
{
    data->enc_diff = get_handwheel_diff();
    data->state = LV_INDEV_STATE_PRESSED;
}

//-----------------------------------------------------------------------------
// Initialize Arduino_GFX, LVGL display & input drivers, and 1 ms tick timer.
// Call once from setup().
//-----------------------------------------------------------------------------
void lvgl_driver_init()
{
    if (driver_ready)
        return;
    driver_ready = true;

    // 1) Initialize LVGL core
    lv_init();

    // 2) Create esp_timer to generate LVGL ticks at 1 kHz
    const esp_timer_create_args_t tick_args = {
        .callback = &lv_tick_task,
        .name = "lv_tick"};
    esp_timer_handle_t tick_timer;
    esp_timer_create(&tick_args, &tick_timer);
    // Period = 1 ms = 1000 µs
    esp_timer_start_periodic(tick_timer, 1000);

    // 3) Initialize the TFT hardware
    tft.begin();
    tft.fillScreen(0x0000);

    if constexpr (PendantConfig::LCD_BL)
    {
        pinMode(PendantConfig::LCD_BL, OUTPUT);
        digitalWrite(PendantConfig::LCD_BL, HIGH);
    }

    // 4) Create LVGL display
    disp = lv_display_create(SCREEN_W, SCREEN_H);
    lv_display_set_driver_data(disp, &tft);

    // 5) Set up double buffering and flush callback
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(
        disp,
        buf1,                 // first buffer
        buf2,                 // second buffer
        SCREEN_W * LINES_BUF, // buffer size in pixels
        LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 6) Register encoder input device
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev, encoder_read_cb);

    // 7) Create an encoder group and link it
    g_input_group = lv_group_create();
    lv_indev_set_group(indev, g_input_group);

    // 8) Initialize FPS timer
    last_ms = millis();
}

//-----------------------------------------------------------------------------
// Query driver initialization status & last FPS.
//-----------------------------------------------------------------------------
bool lvgl_driver_status(int &fps)
{
    if (!driver_ready)
        return false;

    uint32_t now = millis();
    if (now - last_ms >= 1000)
    {
        fps_value = (flush_count * 1000.0f) / (now - last_ms);
        flush_count = 0;
        last_ms = now;
    }
    fps = int(fps_value);
    return true;
}
