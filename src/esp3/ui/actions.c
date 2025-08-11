#include "lvgl.h"
#include "screens.h" // Gives access to the 'objects' struct
#include "actions.h"
#include <stdio.h>

// Include headers for your application's logic
// #include "spindle_control.h"
// #include "gcode_sender.h"

/**
 * @brief A global event handler for objects created in EEZ Studio.
 * * This function is called when an event (like a click) occurs on a widget.
 * We check which object triggered the event and then perform an action.
 */
void action_set_global_eez_event(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    // Example: Handling a click on the "Start" label for macros
    if (target == objects.macro_label_start && code == LV_EVENT_RELEASED)
    { //
        printf("Start Macro button pressed!\n");
        // Call your function to start the selected macro
        // start_selected_macro();
    }

    // Example: Handling a click on the Spindle CW (Clockwise) label
    else if (target == objects.main_label_fr && code == LV_EVENT_RELEASED)
    { //
        printf("Spindle CW pressed!\n");
        // Call your function to turn the spindle on clockwise
        // spindle_run_cw();
    }

    // Example: Handling a click on the X-Axis "RST" label
    else if (target == objects.main_labe_axis_xreset && code == LV_EVENT_PRESSING)
    { //
        printf("Reset X-Axis pressed!\n");
        // Call your function to reset the X-axis coordinate
        // reset_x_axis();
    }

    // Add more 'else if' blocks here for every interactive element...
    // e.g., objects.macro_label_pause, objects.main_label_ff, etc.
}