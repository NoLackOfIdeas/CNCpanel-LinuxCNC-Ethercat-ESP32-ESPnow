/**
 * @file ui_popups.c
 * @brief Implements the logic for the interactive override pop-up windows.
 */

#include "ui_popups.h"
#include "ui/screens.h" // Access to the global `objects` struct from EEZ Studio

// --- Module-static (private) variables ---

static lv_obj_t *active_popup = NULL; // Pointer to the currently visible pop-up
static lv_timer_t *hide_timer = NULL; // Timer for auto-hiding the pop-up

// Timeout in milliseconds
#define POPUP_HIDE_TIMEOUT_MS 2000

// --- Forward declarations for event callbacks ---
static void hide_popup_timer_cb(lv_timer_t *timer);
static void show_popup_event_cb(lv_event_t *e);
static void arc_value_changed_event_cb(lv_event_t *e);
static void background_click_event_cb(lv_event_t *e);

// --- Public Functions ---

void ui_popups_init()
{
    // Attach CLICK events to the labels that will trigger the pop-ups
    lv_obj_add_event_cb(objects.main_label_feed_override_value, show_popup_event_cb, LV_EVENT_CLICKED, objects.window_feed_override);
    lv_obj_add_event_cb(objects.main_label_rapids_feed_override_value, show_popup_event_cb, LV_EVENT_CLICKED, objects.rapids_feed_override);
    lv_obj_add_event_cb(objects.main_label_spindle_feed_override_value, show_popup_event_cb, LV_EVENT_CLICKED, objects.spindle_feed_override);

    // Attach VALUE_CHANGED events to the arcs inside the pop-ups
    lv_obj_add_event_cb(objects.arc_feed_override, arc_value_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.arc_rapids_override_1, arc_value_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.arc_spindle_override, arc_value_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Attach a CLICK event to the main screen's background to detect outside clicks
    lv_obj_add_event_cb(objects.main, background_click_event_cb, LV_EVENT_CLICKED, NULL);
}

// --- Internal Event Callbacks ---

/**
 * @brief Hides the currently active pop-up and deletes the timer.
 */
static void hide_active_popup()
{
    if (active_popup)
    {
        lv_obj_add_flag(active_popup, LV_OBJ_FLAG_HIDDEN);
        active_popup = NULL;
    }
    if (hide_timer)
    {
        lv_timer_del(hide_timer);
        hide_timer = NULL;
    }
}

/**
 * @brief Timer callback that is triggered after the timeout to hide the pop-up.
 */
static void hide_popup_timer_cb(lv_timer_t *timer)
{
    hide_active_popup();
}

/**
 * @brief Event callback triggered when one of the override labels is clicked.
 */
static void show_popup_event_cb(lv_event_t *e)
{
    // Hide any previously active pop-up
    hide_active_popup();

    // Get the window to show from the user_data we attached in init
    lv_obj_t *popup_to_show = (lv_obj_t *)lv_event_get_user_data(e);

    if (popup_to_show)
    {
        active_popup = popup_to_show;
        lv_obj_clear_flag(active_popup, LV_OBJ_FLAG_HIDDEN); // Make it visible
        lv_obj_move_foreground(active_popup);                // Ensure it's on top of other objects

        // Create a one-shot timer to hide the pop-up after the timeout
        hide_timer = lv_timer_create(hide_popup_timer_cb, POPUP_HIDE_TIMEOUT_MS, NULL);
    }
}

/**
 * @brief Event callback triggered when an arc's value changes.
 */
static void arc_value_changed_event_cb(lv_event_t *e)
{
    lv_obj_t *arc = lv_event_get_target(e);
    int32_t value = lv_arc_get_value(arc);

    // Determine which arc was changed and update the corresponding labels
    if (arc == objects.arc_feed_override)
    {
        lv_label_set_text_fmt(objects.arc_feed_override_label, "%d%%", value);
        lv_label_set_text_fmt(objects.main_label_feed_override_value, "%d%%", value);
    }
    else if (arc == objects.arc_rapids_override_1)
    {
        lv_label_set_text_fmt(objects.arc_rapids_override_label_1, "%d%%", value);
        lv_label_set_text_fmt(objects.main_label_rapids_feed_override_value, "%d%%", value);
    }
    else if (arc == objects.arc_spindle_override)
    {
        lv_label_set_text_fmt(objects.arc_spindle_override_label, "%d%%", value);
        lv_label_set_text_fmt(objects.main_label_spindle_feed_override_value, "%d%%", value);
    }

    // Reset the auto-hide timer since there was activity
    if (hide_timer)
    {
        lv_timer_reset(hide_timer);
    }
}

/**
 * @brief Event callback triggered when the main screen background is clicked.
 */
static void background_click_event_cb(lv_event_t *e)
{
    // If the click target is the main background and a pop-up is active, hide it.
    if (lv_event_get_target(e) == objects.main && active_popup)
    {
        hide_active_popup();
    }
}
