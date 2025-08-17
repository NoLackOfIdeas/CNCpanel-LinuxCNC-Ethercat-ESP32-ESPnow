/**
 * @file lvgl_driver.h
 * @brief Public interface for the LVGL driver module.
 *
 * This header declares the functions and variables needed to initialize
 * and interact with the LVGL driver from the main application.
 */

#pragma once

#include <lvgl.h> // Provides all necessary LVGL types (lv_group_t, etc.)

extern bool lvgl_initialized;
extern lv_group_t *g_default_group;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief A global handle to the LVGL input group for the physical encoder.
     *
     * Widgets must be added to this group to be navigable by the handwheel.
     */

    /**
     * @brief Initializes the LVGL display and input drivers using LovyanGFX.
     *
     * This function sets up the display, touchscreen, encoder, and a precise
     * 1ms tick timer. It should be called once from setup().
     */
    void lvgl_driver_init();

    /**
     * @brief (Optional) Query driver status and retrieve last calculated FPS.
     *
     * @param[out] fps A reference to an integer that will be filled with the FPS value.
     * @return true if the driver is initialized, false otherwise.
     */
    bool lvgl_driver_status(int &fps);

#ifdef __cplusplus
}
#endif