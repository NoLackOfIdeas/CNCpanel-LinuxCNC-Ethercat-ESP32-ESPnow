#pragma once

#include "lv_conf.h"
#include <lvgl.h>              // Core LVGL types & functions
#include "shared_structures.h" // struct_message_to_hmi, ButtonBinding, LedBinding, MacroConfig
#include <vector>

// These pointers live in ui.cpp
extern lv_obj_t *jog_screen;
extern lv_obj_t *macro_screen;
extern lv_obj_t *handwheel_btn;
extern lv_obj_t *button_bindings_cont;
extern lv_obj_t *led_bindings_cont;
extern lv_obj_t *macro_list;
extern lv_obj_t *macro_preview;

/**
 * @brief Maximum number of DRO axes the UI can display.
 */
static constexpr uint8_t MAX_DRO_AXES = 6;

/**
 * @brief Number of DRO axes currently in use (1..MAX_DRO_AXES),
 *        driven by runtime/web configuration.
 */
extern uint8_t num_dro_axes;

/**
 * @brief Array of LVGL label objects for each DRO axis.
 *        dro_labels[0] … dro_labels[num_dro_axes-1] are valid.
 */
extern lv_obj_t *dro_labels[MAX_DRO_AXES];

/**
 * @brief Initialize the entire UI subsystem.
 */
void ui_init();

/**
 * @brief Instantiate all LVGL screens and widgets.
 */
void ui_create_screens();

/**
 * @brief Change the active UI to the given screen object.
 */
void ui_set_screen(lv_obj_t *screen);

/**
 * @brief Refresh the Jog screen’s status bar and DRO values.
 */
void ui_update_jog_screen(const struct_message_to_hmi &data,
                          uint8_t axis,
                          uint8_t step);

/**
 * @brief Refresh the Macro screen’s preview list and status indicators.
 */
void ui_update_macro_screen(const struct_message_to_hmi &data);

// --------------------------------------------------
// Prototypes for web-configurable UI settings
// --------------------------------------------------

/**
 * @brief Adjust how many DRO axes to show.
 */
void ui_set_dro_axis_count(int count);

/**
 * @brief Show or hide (enable/disable) the handwheel-enable button.
 */
void ui_set_handwheel_enable_button(bool enabled);

/**
 * @brief Configure the binding between physical buttons and CNC actions.
 */
void ui_configure_button_bindings(const std::vector<ButtonBinding> &bindings);

/**
 * @brief Configure which pendant LEDs map to which CNC/GUI signals.
 */
void ui_configure_led_bindings(const std::vector<LedBinding> &bindings);

/**
 * @brief Upload and apply user-defined macros to the macro screen.
 */
void ui_configure_macros(const std::vector<MacroEntry> &macros);
