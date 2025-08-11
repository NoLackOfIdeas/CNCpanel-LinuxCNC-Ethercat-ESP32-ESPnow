/**
 * @file ui_tab_logic.c
 * @brief Implements the logic for toggling panels based on the active tab.
 */

#include "ui_tab_logic.h"
#include "ui/screens.h" // Access to the global `objects` struct from EEZ Studio

// --- Forward declaration for the event callback ---
static void tab_changed_event_cb(lv_event_t *e);

// --- Public Functions ---

void ui_tab_logic_init()
{
    // Ensure the main tabview object exists before adding an event
    if (objects.main_tabview)
    {
        // Attach an event listener that triggers when the selected tab changes
        lv_obj_add_event_cb(objects.main_tabview, tab_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

        // Manually trigger the event once to set the correct initial state
        lv_obj_send_event(objects.main_tabview, LV_EVENT_VALUE_CHANGED, NULL);
    }
}

// --- Internal Event Callback ---

/**
 * @brief Called whenever the active tab in the main tabview is changed.
 */
static void tab_changed_event_cb(lv_event_t *e)
{
    lv_obj_t *tabview = lv_event_get_target(e);

    // Get the index of the currently active tab (0 for the first tab, 1 for the second, etc.)
    uint32_t active_tab_index = lv_tabview_get_tab_act(tabview);

    // Check if the panel objects exist
    if (objects.main_panel_config && objects.macro_panel_control)
    {
        // The first tab (index 0) is the "Main" tab
        if (active_tab_index == 0)
        {
            // Show the config panel and hide the macro panel
            lv_obj_clear_flag(objects.main_panel_config, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.macro_panel_control, LV_OBJ_FLAG_HIDDEN);
        }
        // The second tab (index 1) is the "Macros" tab
        else if (active_tab_index == 1)
        {
            // Hide the config panel and show the macro panel
            lv_obj_add_flag(objects.main_panel_config, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.macro_panel_control, LV_OBJ_FLAG_HIDDEN);
        }
    }
}