/**
 * @file ui.cpp
 * @brief LVGL-based UI implementation for the Handheld Pendant (ESP3).
 *
 * Implements:
 *   - ui_init(): initialize LVGL, styles, and build all screens
 *   - ui_create_screens(): create screens and widgets
 *   - ui_set_screen(): switch the active screen
 *   - ui_next_screen()/ui_prev_screen(): cycle screens via input
 *   - ui_update_jog_screen(): refresh jog screen (DROs, overrides, step)
 *   - ui_update_macro_screen(): refresh macro screen preview
 *
 * @author Stephan
 * @date 2025-08-04
 * @dependencies LVGL v8.x, lvgl_driver.h, persistence_esp3.h,
 *                communication_esp3.h, shared_structures.h
 */

#include "lv_conf.h"
#include "ui.h"
#include "lvgl_driver.h"
#include "persistence_esp3.h"
#include "communication_esp3.h" // for lcnc_send_action()
#include "shared_structures.h"
#include <cstdio>
#include <cassert>
#include <lvgl.h> // Core LVGL types & init

extern PendantWebConfig pendant_web_cfg;
extern lv_group_t *g_input_group;

// Definitions of extern widget pointers
lv_obj_t *handwheel_btn = nullptr;
lv_obj_t *button_bindings_cont = nullptr;
lv_obj_t *led_bindings_cont = nullptr;
lv_obj_t *macro_list = nullptr;
lv_obj_t *macro_preview = nullptr;
lv_obj_t *jog_screen = nullptr;
lv_obj_t *macro_screen = nullptr;

//-----------------------------------------------------------------------------
// Layout & Style Constants
//-----------------------------------------------------------------------------

static constexpr int SCREEN_WIDTH = 480;
static constexpr int SCREEN_HEIGHT = 320;
static constexpr int STATUS_BAR_HEIGHT = 30;
static constexpr int DRO_PANEL_HEIGHT = 180;
static constexpr int DATA_BAR_HEIGHT = 30;
static constexpr int STEP_BAR_HEIGHT = 80;
static constexpr int TOP_MARGIN = 30;
static constexpr int STATUS_ICON_OFFSET = 10;
static constexpr int STATUS_CENTER_OFFSET = 60;
static constexpr int DATA_BAR_MARGIN = 10;

static const lv_color_t COLOR_BG = lv_color_hex(0x111111);
static const lv_color_t COLOR_BAR_BG = lv_color_hex(0x2C3E50);
static const lv_color_t COLOR_ACTIVE = lv_color_hex(0x00FF00);
static const lv_color_t COLOR_INACTIVE = lv_color_hex(0xCCCCCC);

//-----------------------------------------------------------------------------
// Dynamic DRO axes
//-----------------------------------------------------------------------------

uint8_t num_dro_axes = 4; // overridden in ui_init()

lv_obj_t *dro_labels[MAX_DRO_AXES] = {nullptr};

// Hardcoded defaults, used only if config.axis_labels is missing or too short
static const char axis_names[MAX_DRO_AXES] = {'X', 'Y', 'Z', 'A', 'B', 'C'};

// Holds the runtime labels (from pendant_web_cfg.axis_labels or fallback)
static std::string axis_labels[MAX_DRO_AXES];

//-----------------------------------------------------------------------------
// Other widgets
//-----------------------------------------------------------------------------

static lv_obj_t *feed_label = nullptr;
static lv_obj_t *spindle_label = nullptr;
static lv_obj_t *cutting_speed_label = nullptr;
static lv_obj_t *step_label = nullptr;

//-----------------------------------------------------------------------------
// Style objects
//-----------------------------------------------------------------------------

static lv_style_t style_status_label;
static lv_style_t style_dro_inactive;
static lv_style_t style_dro_active;
static lv_style_t style_data_label;
static lv_style_t style_step_label;

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

static void init_styles();
static void createDROLabels(lv_obj_t *parent);
static lv_obj_t *create_dro_label(lv_obj_t *parent);
static lv_obj_t *create_data_label(lv_obj_t *parent);
static lv_obj_t *create_step_label(lv_obj_t *parent);

static void jog_screen_init();
static void macro_screen_init();
static void macro_btn_event_cb(lv_event_t *e);

//-----------------------------------------------------------------------------
// Status Bar encapsulation
//-----------------------------------------------------------------------------

class StatusBar
{
public:
    void init()
    {
        container = lv_obj_create(lv_layer_top());
        assert(container && "StatusBar::init failed");

        lv_obj_set_size(container, SCREEN_WIDTH, STATUS_BAR_HEIGHT);
        lv_obj_set_style_bg_color(container, COLOR_BAR_BG, LV_PART_MAIN);
        lv_obj_add_flag(container, LV_OBJ_FLAG_FLOATING);

        status_lbl = lv_label_create(container);
        feed_ovr_lbl = lv_label_create(container);
        rapid_ovr_lbl = lv_label_create(container);
        spindle_ovr_lbl = lv_label_create(container);

        assert(status_lbl && feed_ovr_lbl && rapid_ovr_lbl && spindle_ovr_lbl);

        lv_obj_add_style(status_lbl, &style_status_label, LV_PART_MAIN);
        lv_obj_add_style(feed_ovr_lbl, &style_status_label, LV_PART_MAIN);
        lv_obj_add_style(rapid_ovr_lbl, &style_status_label, LV_PART_MAIN);
        lv_obj_add_style(spindle_ovr_lbl, &style_status_label, LV_PART_MAIN);

        lv_obj_align(status_lbl, LV_ALIGN_LEFT_MID, STATUS_ICON_OFFSET, 0);
        lv_obj_align(feed_ovr_lbl, LV_ALIGN_CENTER, -STATUS_CENTER_OFFSET, 0);
        lv_obj_align(rapid_ovr_lbl, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(spindle_ovr_lbl, LV_ALIGN_CENTER, STATUS_CENTER_OFFSET, 0);
    }

    void update(const struct_message_to_hmi &data)
    {
        assert(container && "StatusBar::update before init");

        if (bitRead(data.machine_status, 4))
            lv_label_set_text(status_lbl, LV_SYMBOL_PLAY " RUNNING");
        else if (bitRead(data.machine_status, 5))
            lv_label_set_text(status_lbl, LV_SYMBOL_PAUSE " PAUSED");
        else
            lv_label_set_text(status_lbl, LV_SYMBOL_SETTINGS " IDLE");

        char buf[32];
        std::snprintf(buf, sizeof(buf), "F: %.0f%%", data.feed_override * 100);
        lv_label_set_text(feed_ovr_lbl, buf);

        std::snprintf(buf, sizeof(buf), "R: %.0f%%", data.rapid_override * 100);
        lv_label_set_text(rapid_ovr_lbl, buf);

        std::snprintf(buf, sizeof(buf), "S: %.0f%%", data.spindle_override * 100);
        lv_label_set_text(spindle_ovr_lbl, buf);
    }

private:
    lv_obj_t *container = nullptr;
    lv_obj_t *status_lbl = nullptr;
    lv_obj_t *feed_ovr_lbl = nullptr;
    lv_obj_t *rapid_ovr_lbl = nullptr;
    lv_obj_t *spindle_ovr_lbl = nullptr;
};

static StatusBar statusBar;

//-----------------------------------------------------------------------------
// Screen management
//-----------------------------------------------------------------------------

static constexpr int NUM_SCREENS = 2;
static lv_obj_t *screens[NUM_SCREENS] = {nullptr, nullptr};
static int current_screen = 0;

//-----------------------------------------------------------------------------
// Public API
//-----------------------------------------------------------------------------

void ui_init()
{
    lvgl_driver_init();
    init_styles();

    // 1) Configure DRO axis count
    num_dro_axes = pendant_web_cfg.num_dro_axes;
    assert(num_dro_axes <= MAX_DRO_AXES && "ui_init: num_dro_axes exceeds MAX_DRO_AXES");

    // 2) Load axis labels from web config, fallback to hardcoded
    for (int i = 0; i < MAX_DRO_AXES; ++i)
    {
        if (i < (int)pendant_web_cfg.axis_labels.size())
            axis_labels[i] = pendant_web_cfg.axis_labels[i];
        else
            axis_labels[i] = std::string(1, axis_names[i]);
    }

    // build screens and show first
    ui_create_screens();
    ui_set_screen(screens[0]);
}

void ui_create_screens()
{
    statusBar.init();
    jog_screen_init();
    macro_screen_init();

    assert(screens[0] && screens[1] && "ui_create_screens: screens not created");
}

void ui_set_screen(lv_obj_t *screen)
{
    assert(screen && "ui_set_screen: null screen");
    lv_disp_load_scr(screen);
}

void ui_next_screen()
{
    current_screen = (current_screen + 1) % NUM_SCREENS;
    ui_set_screen(screens[current_screen]);
}

void ui_prev_screen()
{
    current_screen = (current_screen - 1 + NUM_SCREENS) % NUM_SCREENS;
    ui_set_screen(screens[current_screen]);
}

void ui_update_jog_screen(const struct_message_to_hmi &data,
                          uint8_t axis,
                          uint8_t step)
{
    assert(axis < num_dro_axes && "axis out of range");
    statusBar.update(data);

    char buf[64];
    for (int i = 0; i < num_dro_axes; ++i)
    {
        assert(dro_labels[i] && "missing DRO label");

        // pick the label from config or fallback
        const char *lbl_cstr = axis_labels[i].empty()
                                   ? std::string(1, axis_names[i]).c_str()
                                   : axis_labels[i].c_str();

        std::snprintf(buf, sizeof(buf), "%s %+.3f", lbl_cstr, data.dro_pos[i]);
        lv_label_set_text(dro_labels[i], buf);

        bool is_active = (i == axis);
        lv_obj_remove_style_all(dro_labels[i]);
        lv_obj_add_style(dro_labels[i],
                         is_active ? &style_dro_active : &style_dro_inactive,
                         LV_PART_MAIN);
    }

    std::snprintf(buf, sizeof(buf), "F: %.0f%%", data.feed_override * 100);
    lv_label_set_text(feed_label, buf);

    std::snprintf(buf, sizeof(buf), "S: %.0f%%", data.spindle_override * 100);
    lv_label_set_text(spindle_label, buf);

    std::snprintf(buf, sizeof(buf), "C: %.0f", data.cutting_speed);
    lv_label_set_text(cutting_speed_label, buf);

    std::snprintf(buf, sizeof(buf), "STEP: %u", step);
    lv_label_set_text(step_label, buf);
}

void ui_update_macro_screen(const struct_message_to_hmi &data)
{
    assert(macro_preview && "macro_preview missing");
    lv_textarea_set_text(macro_preview, data.macro_text);
}

//-----------------------------------------------------------------------------
// Implementation details
//-----------------------------------------------------------------------------

static void init_styles()
{
    lv_style_init(&style_status_label);
    lv_style_set_text_color(&style_status_label, COLOR_INACTIVE);
    lv_style_set_text_font(&style_status_label, &lv_font_montserrat_20);

    lv_style_init(&style_dro_inactive);
    lv_style_set_text_color(&style_dro_inactive, COLOR_INACTIVE);
    lv_style_set_text_font(&style_dro_inactive, &lv_font_montserrat_32);

    lv_style_init(&style_dro_active);
    lv_style_set_text_color(&style_dro_active, COLOR_ACTIVE);
    lv_style_set_text_font(&style_dro_active, &lv_font_montserrat_40);

    lv_style_init(&style_data_label);
    lv_style_set_text_color(&style_data_label, COLOR_INACTIVE);
    lv_style_set_text_font(&style_data_label, &lv_font_montserrat_20);

    lv_style_init(&style_step_label);
    lv_style_set_text_color(&style_step_label, COLOR_INACTIVE);
    lv_style_set_text_font(&style_step_label, &lv_font_montserrat_20);
    lv_style_set_text_align(&style_step_label, LV_TEXT_ALIGN_CENTER);
}

static void createDROLabels(lv_obj_t *parent)
{
    for (int i = 0; i < MAX_DRO_AXES; ++i)
    {
        if (dro_labels[i])
        {
            lv_obj_del(dro_labels[i]);
            dro_labels[i] = nullptr;
        }
    }
    for (int i = 0; i < num_dro_axes; ++i)
    {
        dro_labels[i] = create_dro_label(parent);
    }
}

static lv_obj_t *create_dro_label(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    assert(lbl && "create_dro_label failed");
    lv_obj_add_style(lbl, &style_dro_inactive, LV_PART_MAIN);
    return lbl;
}

static lv_obj_t *create_data_label(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    assert(lbl && "create_data_label failed");
    lv_obj_add_style(lbl, &style_data_label, LV_PART_MAIN);
    return lbl;
}

static lv_obj_t *create_step_label(lv_obj_t *parent)
{
    lv_obj_t *lbl = lv_label_create(parent);
    assert(lbl && "create_step_label failed");
    lv_obj_add_style(lbl, &style_step_label, LV_PART_MAIN);
    return lbl;
}

static void jog_screen_init()
{
    // Create screen container
    lv_obj_t *screen = lv_obj_create(nullptr);
    screens[0] = screen;
    lv_obj_set_style_bg_color(screen, COLOR_BG, LV_PART_MAIN);
    jog_screen = screen;

    // DRO panel
    lv_obj_t *dro_panel = lv_obj_create(screen);
    lv_obj_set_size(dro_panel, SCREEN_WIDTH, DRO_PANEL_HEIGHT);
    lv_obj_set_flex_flow(dro_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_align(dro_panel, LV_ALIGN_TOP_MID, 0, TOP_MARGIN);
    createDROLabels(dro_panel);

    // Data bar
    lv_obj_t *data_bar = lv_obj_create(screen);
    lv_obj_set_size(data_bar, SCREEN_WIDTH, DATA_BAR_HEIGHT);
    lv_obj_align(data_bar, LV_ALIGN_TOP_MID, 0, DRO_PANEL_HEIGHT + TOP_MARGIN);

    feed_label = create_data_label(data_bar);
    spindle_label = create_data_label(data_bar);
    cutting_speed_label = create_data_label(data_bar);

    lv_obj_align(feed_label, LV_ALIGN_LEFT_MID, DATA_BAR_MARGIN, 0);
    lv_obj_align(spindle_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(cutting_speed_label, LV_ALIGN_RIGHT_MID, -DATA_BAR_MARGIN, 0);

    // Step bar
    lv_obj_t *step_bar = lv_obj_create(screen);
    lv_obj_set_size(step_bar, SCREEN_WIDTH, STEP_BAR_HEIGHT);
    lv_obj_align(step_bar, LV_ALIGN_BOTTOM_MID, 0, 0);

    step_label = create_step_label(step_bar);
    lv_obj_align(step_label, LV_ALIGN_LEFT_MID, DATA_BAR_MARGIN, 0);
}

static void macro_screen_init()
{
    // Create screen container
    lv_obj_t *screen = lv_obj_create(nullptr);
    screens[1] = screen;
    lv_obj_set_style_bg_color(screen, COLOR_BG, LV_PART_MAIN);
    macro_screen = screen;

    // Macro list container
    macro_list = lv_list_create(screen);
    lv_obj_set_size(macro_list, SCREEN_WIDTH, SCREEN_HEIGHT - DRO_PANEL_HEIGHT);
    lv_obj_align(macro_list, LV_ALIGN_TOP_LEFT, 0, 0);

    // Add each macro as a button
    for (size_t i = 0; i < pendant_web_cfg.macros.size(); ++i)
    {
        lv_obj_t *btn = lv_list_add_btn(
            macro_list,
            LV_SYMBOL_FILE,
            pendant_web_cfg.macros[i].name.c_str());
        assert(btn && "macro_screen_init: failed to add macro btn");
        lv_obj_set_user_data(btn, reinterpret_cast<void *>(i));
        lv_obj_add_event_cb(btn, macro_btn_event_cb, LV_EVENT_CLICKED, nullptr);
        lv_group_add_obj(g_input_group, btn);
    }

    // G-code preview textarea
    macro_preview = lv_textarea_create(screen);
    assert(macro_preview && "macro_screen_init: textarea failed");
    lv_obj_set_size(macro_preview, SCREEN_WIDTH, DRO_PANEL_HEIGHT);
    lv_obj_align(macro_preview, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_textarea_set_placeholder_text(
        macro_preview,
        "G-Code will appear here...");
    lv_obj_add_state(macro_preview, LV_STATE_DISABLED);
}

static void macro_btn_event_cb(lv_event_t *e)
{
    assert(e && "macro_btn_event_cb: null event");
    // Cast the target (void*) to lv_obj_t*
    lv_obj_t *btn = reinterpret_cast<lv_obj_t *>(lv_event_get_target(e));
    size_t idx = (size_t)lv_obj_get_user_data(btn);
    assert(idx < pendant_web_cfg.macros.size() &&
           "macro_btn_event_cb: idx out of range");

    // Send press and release
    lcnc_send_action(pendant_web_cfg.macros[idx].action_code, true);
    lcnc_send_action(pendant_web_cfg.macros[idx].action_code, false);
}

//----------------------------------------------------------------------
// ui_set_dro_axis_count
//----------------------------------------------------------------------

void ui_set_dro_axis_count(int count)
{
    if (count < 1)
        count = 1;
    if (count > MAX_DRO_AXES)
        count = MAX_DRO_AXES;

    num_dro_axes = static_cast<uint8_t>(count);
    for (int i = 0; i < MAX_DRO_AXES; ++i)
    {
        if (i < num_dro_axes)
            lv_obj_clear_flag(dro_labels[i], LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(dro_labels[i], LV_OBJ_FLAG_HIDDEN);
    }
}

//----------------------------------------------------------------------
// ui_set_handwheel_enable_button
//----------------------------------------------------------------------

void ui_set_handwheel_enable_button(bool on)
{
    if (!handwheel_btn)
        return;
    if (on)
        lv_obj_clear_flag(handwheel_btn, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(handwheel_btn, LV_OBJ_FLAG_HIDDEN);
}

//----------------------------------------------------------------------
// ui_configure_button_bindings
//----------------------------------------------------------------------

void ui_configure_button_bindings(const std::vector<ButtonBinding> &bindings)
{
    if (!button_bindings_cont)
        return;
    lv_obj_clean(button_bindings_cont);

    for (const auto &b : bindings)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf),
                      "Button %u: %s",
                      b.button_index,
                      b.action_name.c_str());
        lv_obj_t *lbl = lv_label_create(button_bindings_cont);
        lv_label_set_text(lbl, buf);
    }
}

//----------------------------------------------------------------------
// ui_configure_led_bindings
//----------------------------------------------------------------------

void ui_configure_led_bindings(const std::vector<LedBinding> &bindings)
{
    if (!led_bindings_cont)
        return;
    lv_obj_clean(led_bindings_cont);

    for (const auto &l : bindings)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf),
                      "LED %u: %s",
                      l.led_index,
                      l.signal_name.c_str());
        lv_obj_t *lbl = lv_label_create(led_bindings_cont);
        lv_label_set_text(lbl, buf);
    }
}

//----------------------------------------------------------------------
// ui_configure_macros
//----------------------------------------------------------------------

void ui_configure_macros(const std::vector<MacroEntry> &macros)
{
    if (!macro_list || !macro_preview)
        return;

    // Clear old list
    lv_obj_clean(macro_list);

    // Add each macro
    for (const auto &m : macros)
    {
        lv_list_add_btn(macro_list, LV_SYMBOL_FILE, m.name.c_str());
    }

    // Preview first macro's commands
    if (!macros.empty())
    {
        std::string combined;
        for (const auto &cmd : macros[0].commands)
        {
            combined += cmd;
            combined += '\n';
        }
        lv_label_set_text(macro_preview, combined.c_str());
    }
}
