/**
 * @file lvgl_driver.cpp
 * @brief LVGL v9+ display and input driver for the ESP32-S3 Pendant.
 */

#include "lvgl_driver.h"
#include "hmi_handler_esp3.h"
#include <lvgl.h>
#include "LGFX_Config.h"
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include "persistence_esp3.h"

// --- Driver Instances and Globals ---
static LGFX tft;
static lv_display_t *disp;
lv_group_t *g_default_group = nullptr;
extern QueueHandle_t encoderDeltaQueue;

// --- LVGL Callbacks ---

static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

static void display_flush_cb(lv_display_t *display,
                             const lv_area_t *area,
                             uint8_t *px_map)
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    tft.pushImage(area->x1, area->y1, w, h, (const uint16_t *)px_map);
    lv_display_flush_ready(display);
}

static void touchpad_read_cb(lv_indev_t *indev,
                             lv_indev_data_t *data)
{
    (void)indev;
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);
    if (touched)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touchX;
        data->point.y = touchY;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    int32_t diff = get_handwheel_diff();
    // apply inversion and deadzone as before…
    if (encoder_inverted)
        diff = -diff;
    if (abs(diff) < encoder_deadzone)
        diff = 0;

    // feed LVGL so the UI still moves
    data->enc_diff = diff;

    // queue it for later logging/processing
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(encoderDeltaQueue, &diff, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// --- Main Initialization Function ---

void lvgl_driver_init()
{
    // 1. Hardware + LVGL core
    tft.begin();
    lv_init();

    // 2. 1 ms tick via esp_timer
    const esp_timer_create_args_t tick_timer_args = {
        .callback = &lv_tick_task,
        .name = "lv_tick_timer"};
    esp_timer_handle_t tick_timer;
    esp_timer_create(&tick_timer_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, 1000);

    // 3. Create display
    disp = lv_display_create(tft.width(), tft.height());
    lv_display_set_flush_cb(disp, display_flush_cb);

    // 4. Allocate 32-row buffers in PSRAM (frees internal DMA heap)
    const size_t buffer_rows = 32;
    const size_t buffer_size = tft.width() * buffer_rows;

    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(
        buffer_size * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf1)
    {
        while (true)
        { /* PSRAM allocation failed – halt */
        }
    }

    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(
        buffer_size * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf2)
    {
        while (true)
        { /* PSRAM allocation failed – halt */
        }
    }

    // Performance instrumentation: dump free PSRAM vs internal heap
    heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    heap_caps_print_heap_info(MALLOC_CAP_8BIT);

    lv_display_set_buffers(
        disp,
        buf1, buf2,
        buffer_size,
        LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 5. Register touch as a pointer device
    lv_indev_t *indev_touch = lv_indev_create();
    lv_indev_set_type(indev_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touch, touchpad_read_cb);

    // 6. Register handwheel encoder only (no keyboard)
    lv_indev_t *indev_encoder = lv_indev_create();
    lv_indev_set_type(indev_encoder, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_encoder, encoder_read_cb);

    // 7. Create and assign an encoder-only group
    g_default_group = lv_group_create();
    lv_group_set_default(g_default_group);
    lv_indev_set_group(indev_encoder, g_default_group);
}
