/**
 * @file ui_tab_logic.h
 * @brief Public interface for managing custom tab view logic.
 */

#ifndef UI_TAB_LOGIC_H
#define UI_TAB_LOGIC_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initializes the event handlers for the main tab view.
     *
     * This function attaches an LVGL event to the tab view to toggle the visibility
     * of the control panels based on the selected tab.
     *
     * Call this once from setup() after the main UI has been initialized.
     */
    void ui_tab_logic_init();

#ifdef __cplusplus
}
#endif

#endif // UI_TAB_LOGIC_H