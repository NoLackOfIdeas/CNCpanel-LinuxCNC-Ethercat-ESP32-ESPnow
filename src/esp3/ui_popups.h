
/**
 * @file ui_popups.h
 * @brief Public interface for managing the interactive override pop-up windows.
 */

#ifndef UI_POPUPS_H
#define UI_POPUPS_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initializes the event handlers for the override pop-up windows.
     *
     * This function attaches all the necessary LVGL events to the labels and arcs
     * to handle showing, hiding, and updating the pop-up windows.
     *
     * Call this once from setup() after the main UI has been initialized.
     */
    void ui_popups_init();

#ifdef __cplusplus
}
#endif

#endif // UI_POPUPS_H
