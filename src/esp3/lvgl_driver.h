#pragma once

#include "lv_conf.h"
#include <lvgl.h> // LVGL core types
#include <stdint.h>
#include <esp_timer.h> // For esp_timer_handle_t

#ifdef __cplusplus
extern "C"
{
#endif

    // Global LVGL encoder group (used for navigation focus)
    extern lv_group_t *g_input_group;

    // Internal esp_timer handle for LVGL tick (created in init)
    extern esp_timer_handle_t lvgl_tick_timer;

    /**
     * @brief Initialize LVGL display and input drivers.
     *        Call once from setup() after lv_init().
     *        Also starts esp_timer for 1 ms LVGL ticks.
     */
    void lvgl_driver_init();

    /**
     * @brief LVGL v9 flush callback: pushes rendered area to display.
     * @param disp LVGL display handle
     * @param area Area to update
     * @param color_buf Pixel buffer (RGB565)
     */
    void my_disp_flush(lv_display_t *, const lv_area_t *, uint8_t *);

    /**
     * @brief LVGL v9 encoder read callback.
     * @param indev Input device handle
     * @param data Encoder state and delta
     */
    void encoder_read_cb(lv_indev_t *, lv_indev_data_t *);

    /**
     * @brief Query driver status and retrieve last FPS.
     * @param fps Output: last calculated frames per second
     * @return true if driver is initialized
     */
    bool lvgl_driver_status(int &fps);

#ifdef __cplusplus
}
#endif