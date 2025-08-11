#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            // MainPanelStatus
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_panel_status = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 60);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // MainLabelPendantEnable
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_pendant_enable = obj;
                    lv_obj_set_pos(obj, -7, -9);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "Pendant");
                }
                {
                    // MainSwitchPendant
                    lv_obj_t *obj = lv_switch_create(parent_obj);
                    objects.main_switch_pendant = obj;
                    lv_obj_set_pos(obj, 58, -9);
                    lv_obj_set_size(obj, 20, 14);
                    lv_obj_add_state(obj, LV_STATE_CHECKED);
                }
                {
                    // MainLabelLCNCEnable
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_lcnc_enable = obj;
                    lv_obj_set_pos(obj, -7, 17);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "LCNC");
                }
                {
                    // MainSwitchLCNC
                    lv_obj_t *obj = lv_switch_create(parent_obj);
                    objects.main_switch_lcnc = obj;
                    lv_obj_set_pos(obj, 58, 17);
                    lv_obj_set_size(obj, 20, 14);
                    lv_obj_add_state(obj, LV_STATE_CHECKED);
                }
                {
                    // MainLabelFeed
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_feed = obj;
                    lv_obj_set_pos(obj, 98, -9);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Feed");
                }
                {
                    // MainLabelRapids
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_rapids = obj;
                    lv_obj_set_pos(obj, 249, 15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Rapids");
                }
                {
                    // MainLabelSpindle
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_spindle = obj;
                    lv_obj_set_pos(obj, 249, -9);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Spindle");
                }
                {
                    // MainLabelBatPercent
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_bat_percent = obj;
                    lv_obj_set_pos(obj, 395, -9);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "100%");
                }
                {
                    // MainSymWifi
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_sym_wifi = obj;
                    lv_obj_set_pos(obj, 430, 15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "\uF1EB");
                }
                {
                    // MainSymBat
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_sym_bat = obj;
                    lv_obj_set_pos(obj, 430, -9);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "\uF240");
                }
                {
                    // MainLabelCut
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_cut = obj;
                    lv_obj_set_pos(obj, 98, 15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Cut");
                }
                {
                    // MainLabelFeedValue
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_feed_value = obj;
                    lv_obj_set_pos(obj, 140, -9);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "1500 mm/min");
                }
                {
                    // MainLabelCutValue
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_cut_value = obj;
                    lv_obj_set_pos(obj, 140, 15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "196 m/min");
                }
                {
                    // MainLabelSpindleValue
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_spindle_value = obj;
                    lv_obj_set_pos(obj, 309, -9);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "2500 rpm");
                }
                {
                    // MainLabelRapids_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_rapids_1 = obj;
                    lv_obj_set_pos(obj, 309, 15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "8.0 m/min");
                }
            }
        }
        {
            // MainPanelOverrides
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_panel_overrides = obj;
            lv_obj_set_pos(obj, 0, 229);
            lv_obj_set_size(obj, 480, 45);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // MainLabelFeedOverride
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_feed_override = obj;
                    lv_obj_set_pos(obj, 4, -13);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "Feed-Override");
                }
                {
                    // MainLabelFeedOverrideValue
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_feed_override_value = obj;
                    lv_obj_set_pos(obj, 42, -2);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "80%");
                }
                {
                    // MainLabelRapidsFeedOverride
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_rapids_feed_override = obj;
                    lv_obj_set_pos(obj, 163, -13);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "Rapids-Override");
                }
                {
                    // MainLabelRapidsFeedOverrideValue
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_rapids_feed_override_value = obj;
                    lv_obj_set_pos(obj, 199, -2);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "110%");
                }
                {
                    // MainLabelSpindleOverride
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_spindle_override = obj;
                    lv_obj_set_pos(obj, 323, -13);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "Spindle-Override");
                }
                {
                    // MainLabelSpindleFeedOverrideValue
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_spindle_feed_override_value = obj;
                    lv_obj_set_pos(obj, 359, -2);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "100%");
                }
            }
        }
        {
            // MacroPanelControl
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.macro_panel_control = obj;
            lv_obj_set_pos(obj, 0, 275);
            lv_obj_set_size(obj, 480, 45);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // MacroLabelPause
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.macro_label_pause = obj;
                    lv_obj_set_pos(obj, 173, -6);
                    lv_obj_set_size(obj, 87, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Pause \uF04C");
                }
                {
                    // MacroButtonExecute
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.macro_button_execute = obj;
                    lv_obj_set_pos(obj, -7, -13);
                    lv_obj_set_size(obj, 114, 32);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Start Macro");
                        }
                    }
                }
                {
                    // MacroLabelStart
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.macro_label_start = obj;
                    lv_obj_set_pos(obj, -7, -6);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Start \uF04B");
                }
                {
                    // MacroLabelStop
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.macro_label_stop = obj;
                    lv_obj_set_pos(obj, 324, -6);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "STOP/ESC \uF04D");
                }
            }
        }
        {
            // MainPanelConfig
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_panel_config = obj;
            lv_obj_set_pos(obj, 0, 275);
            lv_obj_set_size(obj, 480, 45);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // MainDropdownJogstep
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.main_dropdown_jogstep = obj;
                    lv_obj_set_pos(obj, 50, -21);
                    lv_obj_set_size(obj, 99, LV_SIZE_CONTENT);
                    lv_dropdown_set_options(obj, "0.001  \n0.01  \n0.1  \n1  \nACC");
                    lv_dropdown_set_selected(obj, 4);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_PRESSED, (void *)0);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_FOCUSED);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_line_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_line_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff282b30), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff282b30), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // MainLabelJogstep
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_jogstep = obj;
                    lv_obj_set_pos(obj, -7, -4);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "Jog-Step");
                }
                {
                    // MainLabeSpindleControl
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_labe_spindle_control = obj;
                    lv_obj_set_pos(obj, -19, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Spindle");
                }
                {
                    // MainLabelFF
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_ff = obj;
                    lv_obj_set_pos(obj, 244, -7);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "CCW");
                }
                {
                    // MainLabelStop
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_stop = obj;
                    lv_obj_set_pos(obj, 329, -7);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "\uF04D / \uF04B");
                }
                {
                    // MainLabelFR
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_label_fr = obj;
                    lv_obj_set_pos(obj, 416, -7);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "CW");
                }
            }
        }
        {
            // MainTabview
            lv_obj_t *obj = lv_tabview_create(parent_obj);
            objects.main_tabview = obj;
            lv_obj_set_pos(obj, 0, 60);
            lv_obj_set_size(obj, 480, 169);
            lv_tabview_set_tab_bar_position(obj, LV_DIR_TOP);
            lv_tabview_set_tab_bar_size(obj, 26);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // MainTab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Main");
                    objects.main_tab = obj;
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_PRESSING, (void *)0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        static lv_coord_t dsc[] = {LV_GRID_TEMPLATE_LAST};
                        lv_obj_set_style_grid_row_dsc_array(obj, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
                    }
                    lv_obj_set_style_grid_cell_x_align(obj, LV_GRID_ALIGN_END, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_END, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // MainLabelAxisX
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_x = obj;
                            lv_obj_set_pos(obj, -5, -1);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "X");
                        }
                        {
                            // MainLabelAxisXValue
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_xvalue = obj;
                            lv_obj_set_pos(obj, 9, -1);
                            lv_obj_set_size(obj, 110, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "2156.204");
                        }
                        {
                            // MainLabelAxisXUnit
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_xunit = obj;
                            lv_obj_set_pos(obj, 121, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // MainLabeAxisXReset
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_labe_axis_xreset = obj;
                            lv_obj_set_pos(obj, 166, 8);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_PRESSING, (void *)0);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "RST");
                        }
                        {
                            // MainLabelAxisY
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_y = obj;
                            lv_obj_set_pos(obj, -5, 41);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Y");
                        }
                        {
                            // MainLabelAxisYValue
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_yvalue = obj;
                            lv_obj_set_pos(obj, 15, 41);
                            lv_obj_set_size(obj, 104, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "556.204");
                        }
                        {
                            // MainLabelAxisYUnit
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_yunit = obj;
                            lv_obj_set_pos(obj, 121, 49);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // MainLabeAxisYReset
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_labe_axis_yreset = obj;
                            lv_obj_set_pos(obj, 166, 50);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_PRESSING, (void *)0);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "RST");
                        }
                        {
                            // MainLabelAxisZ
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_z = obj;
                            lv_obj_set_pos(obj, -5, 81);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Z");
                        }
                        {
                            // MainLabelAxisZValue
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_zvalue = obj;
                            lv_obj_set_pos(obj, 15, 81);
                            lv_obj_set_size(obj, 104, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "6.115");
                        }
                        {
                            // MainLabelAxisZUnit
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_zunit = obj;
                            lv_obj_set_pos(obj, 121, 89);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // MainLabeAxisZReset_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_labe_axis_zreset_1 = obj;
                            lv_obj_set_pos(obj, 166, 90);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_PRESSING, (void *)0);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "RST");
                        }
                        {
                            // MainLabelAxisA
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_a = obj;
                            lv_obj_set_pos(obj, 235, -1);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "A");
                        }
                        {
                            // MainLabelAxisAValue
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_avalue = obj;
                            lv_obj_set_pos(obj, 253, -1);
                            lv_obj_set_size(obj, 102, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "181.42");
                        }
                        {
                            // MainLabelAxisAUnit
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_aunit = obj;
                            lv_obj_set_pos(obj, 361, 7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "deg");
                        }
                        {
                            // MainLabeAxisAReset
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_labe_axis_areset = obj;
                            lv_obj_set_pos(obj, 406, 8);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_PRESSING, (void *)0);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "RST");
                        }
                        {
                            // MainLabelAxisB
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_b = obj;
                            lv_obj_set_pos(obj, 235, 41);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "B");
                        }
                        {
                            // MainLabelAxisBValue
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_bvalue = obj;
                            lv_obj_set_pos(obj, 246, 41);
                            lv_obj_set_size(obj, 109, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "856.204");
                        }
                        {
                            // MainLabelAxisBUnit
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_bunit = obj;
                            lv_obj_set_pos(obj, 361, 49);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // MainLabeAxisBReset
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_labe_axis_breset = obj;
                            lv_obj_set_pos(obj, 406, 50);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_PRESSING, (void *)0);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "RST");
                        }
                        {
                            // MainLabelAxisC
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_c = obj;
                            lv_obj_set_pos(obj, 235, 81);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "C");
                        }
                        {
                            // MainLabelAxisCValue
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_cvalue = obj;
                            lv_obj_set_pos(obj, 253, 81);
                            lv_obj_set_size(obj, 102, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &ui_font_montserat_bold_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_GRID, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                static lv_coord_t dsc[] = {LV_GRID_TEMPLATE_LAST};
                                lv_obj_set_style_grid_row_dsc_array(obj, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
                            }
                            {
                                static lv_coord_t dsc[] = {LV_GRID_TEMPLATE_LAST};
                                lv_obj_set_style_grid_column_dsc_array(obj, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
                            }
                            lv_obj_set_style_grid_cell_x_align(obj, LV_GRID_ALIGN_END, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_grid_column_align(obj, LV_GRID_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "4111.954");
                        }
                        {
                            // MainLabelAxisCUnit
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_label_axis_cunit = obj;
                            lv_obj_set_pos(obj, 361, 89);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "mm");
                        }
                        {
                            // MainLabeAxisCReset
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.main_labe_axis_creset = obj;
                            lv_obj_set_pos(obj, 406, 90);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_PRESSING, (void *)0);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "RST");
                        }
                    }
                }
                {
                    // MacroTab
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Macros");
                    objects.macro_tab = obj;
                    lv_obj_add_event_cb(obj, action_set_global_eez_event, LV_EVENT_PRESSING, (void *)0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // MacrosRoller
                            lv_obj_t *obj = lv_roller_create(parent_obj);
                            objects.macros_roller = obj;
                            lv_obj_set_pos(obj, -16, -16);
                            lv_obj_set_size(obj, 240, 143);
                            lv_roller_set_options(obj, "Macro 1: Tool Change\nMacro 2: Probe Z-Axis\nMacro 3: Go to Park Position\nMacro 4: Center on Fixture\nMacro 5: Warm-up Spindle \nMacro 6: ....", LV_ROLLER_MODE_NORMAL);
                            lv_roller_set_selected(obj, 3, LV_ANIM_OFF);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff15171a), LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                        {
                            // MacrosGcode
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.macros_gcode = obj;
                            lv_obj_set_pos(obj, 224, -16);
                            lv_obj_set_size(obj, 240, 143);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_text(obj, "G91 G28 Z0\nG90\nM6 T[Current_Tool+1]\n...");
                            lv_textarea_set_one_line(obj, false);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff15171a), LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                    }
                }
            }
        }
        {
            // WindowFeedOverride
            lv_obj_t *obj = lv_win_create(parent_obj);
            objects.window_feed_override = obj;
            lv_obj_set_pos(obj, 36, 12);
            lv_obj_set_size(obj, 169, 232);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_state(obj, LV_STATE_DISABLED);
            lv_obj_set_style_bg_opa(obj, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0d0d0d), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ArcFeedOverride
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.arc_feed_override = obj;
                    lv_obj_set_pos(obj, 10, 0);
                    lv_obj_set_size(obj, 160, 181);
                    lv_arc_set_range(obj, 50, 150);
                    lv_arc_set_value(obj, 100);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ArcFeedOverrideLabel
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.arc_feed_override_label = obj;
                            lv_obj_set_pos(obj, 42, 66);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "100%");
                        }
                    }
                }
            }
        }
        {
            // RapidsFeedOverride
            lv_obj_t *obj = lv_win_create(parent_obj);
            objects.rapids_feed_override = obj;
            lv_obj_set_pos(obj, 157, 12);
            lv_obj_set_size(obj, 169, 232);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_state(obj, LV_STATE_DISABLED);
            lv_obj_set_style_bg_opa(obj, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0d0d0d), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ArcRapidsOverride_1
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.arc_rapids_override_1 = obj;
                    lv_obj_set_pos(obj, 10, 0);
                    lv_obj_set_size(obj, 160, 181);
                    lv_arc_set_range(obj, 50, 150);
                    lv_arc_set_value(obj, 100);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ArcRapidsOverrideLabel_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.arc_rapids_override_label_1 = obj;
                            lv_obj_set_pos(obj, 42, 66);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "100%");
                        }
                    }
                }
            }
        }
        {
            // SpindleFeedOverride
            lv_obj_t *obj = lv_win_create(parent_obj);
            objects.spindle_feed_override = obj;
            lv_obj_set_pos(obj, 277, 12);
            lv_obj_set_size(obj, 169, 232);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_state(obj, LV_STATE_DISABLED);
            lv_obj_set_style_bg_opa(obj, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0d0d0d), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // ArcSpindleOverride
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.arc_spindle_override = obj;
                    lv_obj_set_pos(obj, 10, 0);
                    lv_obj_set_size(obj, 160, 181);
                    lv_arc_set_range(obj, 50, 150);
                    lv_arc_set_value(obj, 100);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // ArcSpindleOverrideLabel
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.arc_spindle_override_label = obj;
                            lv_obj_set_pos(obj, 42, 66);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "100%");
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
}
