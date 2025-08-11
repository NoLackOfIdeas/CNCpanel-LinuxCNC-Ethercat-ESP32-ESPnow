#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *main_panel_status;
    lv_obj_t *main_label_pendant_enable;
    lv_obj_t *main_switch_pendant;
    lv_obj_t *main_label_lcnc_enable;
    lv_obj_t *main_switch_lcnc;
    lv_obj_t *main_label_feed;
    lv_obj_t *main_label_rapids;
    lv_obj_t *main_label_spindle;
    lv_obj_t *main_label_bat_percent;
    lv_obj_t *main_sym_wifi;
    lv_obj_t *main_sym_bat;
    lv_obj_t *main_label_cut;
    lv_obj_t *main_label_feed_value;
    lv_obj_t *main_label_cut_value;
    lv_obj_t *main_label_spindle_value;
    lv_obj_t *main_label_rapids_1;
    lv_obj_t *main_panel_overrides;
    lv_obj_t *main_label_feed_override;
    lv_obj_t *main_label_feed_override_value;
    lv_obj_t *main_label_rapids_feed_override;
    lv_obj_t *main_label_rapids_feed_override_value;
    lv_obj_t *main_label_spindle_override;
    lv_obj_t *main_label_spindle_feed_override_value;
    lv_obj_t *macro_panel_control;
    lv_obj_t *macro_label_pause;
    lv_obj_t *macro_button_execute;
    lv_obj_t *macro_label_start;
    lv_obj_t *macro_label_stop;
    lv_obj_t *main_panel_config;
    lv_obj_t *main_dropdown_jogstep;
    lv_obj_t *main_label_jogstep;
    lv_obj_t *main_labe_spindle_control;
    lv_obj_t *main_label_ff;
    lv_obj_t *main_label_stop;
    lv_obj_t *main_label_fr;
    lv_obj_t *main_tabview;
    lv_obj_t *main_tab;
    lv_obj_t *main_label_axis_x;
    lv_obj_t *main_label_axis_xvalue;
    lv_obj_t *main_label_axis_xunit;
    lv_obj_t *main_labe_axis_xreset;
    lv_obj_t *main_label_axis_y;
    lv_obj_t *main_label_axis_yvalue;
    lv_obj_t *main_label_axis_yunit;
    lv_obj_t *main_labe_axis_yreset;
    lv_obj_t *main_label_axis_z;
    lv_obj_t *main_label_axis_zvalue;
    lv_obj_t *main_label_axis_zunit;
    lv_obj_t *main_labe_axis_zreset_1;
    lv_obj_t *main_label_axis_a;
    lv_obj_t *main_label_axis_avalue;
    lv_obj_t *main_label_axis_aunit;
    lv_obj_t *main_labe_axis_areset;
    lv_obj_t *main_label_axis_b;
    lv_obj_t *main_label_axis_bvalue;
    lv_obj_t *main_label_axis_bunit;
    lv_obj_t *main_labe_axis_breset;
    lv_obj_t *main_label_axis_c;
    lv_obj_t *main_label_axis_cvalue;
    lv_obj_t *main_label_axis_cunit;
    lv_obj_t *main_labe_axis_creset;
    lv_obj_t *macro_tab;
    lv_obj_t *macros_roller;
    lv_obj_t *macros_gcode;
    lv_obj_t *window_feed_override;
    lv_obj_t *arc_feed_override;
    lv_obj_t *arc_feed_override_label;
    lv_obj_t *rapids_feed_override;
    lv_obj_t *arc_rapids_override_1;
    lv_obj_t *arc_rapids_override_label_1;
    lv_obj_t *spindle_feed_override;
    lv_obj_t *arc_spindle_override;
    lv_obj_t *arc_spindle_override_label;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
};

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/