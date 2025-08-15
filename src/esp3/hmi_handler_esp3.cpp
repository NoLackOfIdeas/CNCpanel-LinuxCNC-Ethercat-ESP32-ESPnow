/**
 * @file hmi_handler_esp3.cpp
 * @brief Handles all physical HMI inputs (buttons, encoders, selectors) for the ESP32-S3 Pendant.
 */

#include "hmi_handler_esp3.h"
#include "config_esp3.h"
using namespace Pinout;

#include "persistence_esp3.h"
#include "ui.h"
#include <Arduino.h>
#include <ESP32Encoder.h>

// -----------------------------------------------------------------------------
// Encoder instances must live in global scope to avoid PCNT/FreeRTOS before-init
// -----------------------------------------------------------------------------
static ESP32Encoder feedEnc;
static ESP32Encoder rapidEnc;
static ESP32Encoder spindleEnc;

// -----------------------------------------------------------------------------

// --- Constants ---
static constexpr unsigned long KEY_DEBOUNCE_MS = 50;

// --- Internal State Enums and Structs ---
enum class KeyState
{
    IDLE,
    PRESSED,
    HELD,
    RELEASED
};

struct KeyInfo
{
    KeyState state = KeyState::IDLE;
    bool last_raw = false;
    unsigned long last_change_ms = 0;
};

// --- Module-static (private) variables ---
#if PENDANT_HAS_BUTTON_MATRIX
static KeyInfo key_matrix[PENDANT_MATRIX_ROWS][PENDANT_MATRIX_COLS];
#endif

static uint32_t current_button_bitmask = 0;
static ESP32Encoder handwheel;
static int32_t handwheel_position = 0;
static uint8_t selected_axis = 0;
static uint8_t selected_step = 0;
static bool data_changed_flag = false;

// For get_handwheel_diff()
static int32_t last_count = 0;
static volatile int16_t handwheel_diff = 0;

// --- Forward declarations for internal functions ---
static void update_keypad_states();
static void read_encoders();
static void read_selectors();

//================================================================================
// PUBLIC API FUNCTIONS
//================================================================================

void hmi_pendant_init()
{
    Serial.begin(115200);
    Serial.println("\n--- PIN MAP DUMP ---");

#if PENDANT_HAS_BUTTON_MATRIX
    Serial.printf("Rows:   ");
    for (size_t i = 0; i < PENDANT_MATRIX_ROWS; ++i)
        Serial.printf("%u ", PENDANT_ROW_PINS[i]);

    Serial.printf("\nCols:   ");
    for (size_t i = 0; i < PENDANT_MATRIX_COLS; ++i)
        Serial.printf("%u ", PENDANT_COL_PINS[i]);
#endif

#if PENDANT_HAS_LEDS
    Serial.printf("\nLEDs:   ");
    for (size_t i = 0; i < NUM_PENDANT_LEDS; ++i)
        Serial.printf("%u ", PENDANT_LED_PINS[i]);
#endif

#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
    Serial.printf("\nFEED A      -> GPIO %u\n", PIN_FEED_OVR_A);
    Serial.printf("FEED B      -> GPIO %u\n", PIN_FEED_OVR_B);
#endif

#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
    Serial.printf("RAPID A     -> GPIO %u\n", PIN_RAPID_OVR_A);
    Serial.printf("RAPID B     -> GPIO %u\n", PIN_RAPID_OVR_B);
#endif

#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
    Serial.printf("SPINDLE A   -> GPIO %u\n", PIN_SPINDLE_OVR_A);
    Serial.printf("SPINDLE B   -> GPIO %u\n", PIN_SPINDLE_OVR_B);
#endif

    Serial.println("\n--------------------------------\n");

    // 1) Feed-override encoder
#if PENDANT_HAS_FEED_OVERRIDE_ENCODER
    pinMode(PIN_FEED_OVR_A, INPUT_PULLUP);
    pinMode(PIN_FEED_OVR_B, INPUT_PULLUP);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    feedEnc.attachFullQuad(PIN_FEED_OVR_A, PIN_FEED_OVR_B);
    feedEnc.clearCount();
#endif

    // 2) Rapid-override encoder
#if PENDANT_HAS_RAPID_OVERRIDE_ENCODER
    pinMode(PIN_RAPID_OVR_A, INPUT_PULLUP);
    pinMode(PIN_RAPID_OVR_B, INPUT_PULLUP);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    rapidEnc.attachFullQuad(PIN_RAPID_OVR_A, PIN_RAPID_OVR_B);
    rapidEnc.clearCount();
#endif

    // 3) Spindle-override encoder
#if PENDANT_HAS_SPINDLE_OVERRIDE_ENCODER
    pinMode(PIN_SPINDLE_OVR_A, INPUT_PULLUP);
    pinMode(PIN_SPINDLE_OVR_B, INPUT_PULLUP);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    spindleEnc.attachFullQuad(PIN_SPINDLE_OVR_A, PIN_SPINDLE_OVR_B);
    spindleEnc.clearCount();
#endif

    // 4) Handwheel encoder
#if PENDANT_HAS_HANDWHEEL_ENCODER
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    handwheel.attachFullQuad(HW_ENCODER_A, HW_ENCODER_B);
    handwheel.clearCount();
#endif

    // 5) Button matrix
#if PENDANT_HAS_BUTTON_MATRIX
    for (size_t i = 0; i < PENDANT_MATRIX_ROWS; ++i)
    {
        pinMode(PENDANT_ROW_PINS[i], OUTPUT);
        digitalWrite(PENDANT_ROW_PINS[i], HIGH);
    }
    for (size_t i = 0; i < PENDANT_MATRIX_COLS; ++i)
    {
        pinMode(PENDANT_COL_PINS[i], INPUT_PULLUP);
    }
#endif

    // 6) LEDs
#if PENDANT_HAS_LEDS
    for (size_t i = 0; i < NUM_PENDANT_LEDS; ++i)
    {
        pinMode(PENDANT_LED_PINS[i], OUTPUT);
        digitalWrite(PENDANT_LED_PINS[i], LOW);
    }
#endif
}

void hmi_pendant_task()
{
    update_keypad_states();
    read_encoders();
    read_selectors();
    ui_bridge_update_jog_selectors(selected_axis, selected_step);
}

bool hmi_pendant_data_has_changed()
{
    if (data_changed_flag)
    {
        data_changed_flag = false;
        return true;
    }
    return false;
}

void get_pendant_data(PendantStatePacket *out)
{
    if (!out)
        return;
    out->button_states = current_button_bitmask;
    out->handwheel_position = handwheel_position;
    out->selected_axis = selected_axis;
    out->selected_step = selected_step;
}

void update_hmi_from_lcnc(const LcncStatusPacket &data)
{
    ui_bridge_update_from_lcnc(data);

#if PENDANT_HAS_LEDS
    for (size_t i = 0;
         i < pendant_web_cfg.led_bindings.size() && i < NUM_PENDANT_LEDS;
         ++i)
    {
        bool on = false;
        // (Your LED logic here)
        digitalWrite(PENDANT_LED_PINS[i], on ? HIGH : LOW);
    }
#endif
}

void get_pendant_live_status(uint32_t &btn_states,
                             int32_t &hw_pos,
                             uint8_t &axis_pos,
                             uint8_t &step_pos)
{
    btn_states = current_button_bitmask;
    hw_pos = handwheel_position;
    axis_pos = selected_axis;
    step_pos = selected_step;
}

// Returns the delta since last call and stores it internally
int32_t get_handwheel_diff()
{
    int32_t current = handwheel.getCount();
    int32_t diff = current - last_count;
    last_count = current;
    handwheel_diff = diff;
    return diff;
}

//================================================================================
// INTERNAL HELPER FUNCTIONS
//================================================================================

static void update_keypad_states()
{
#if PENDANT_HAS_BUTTON_MATRIX
    // Drive one row low at a time, read columns
    for (size_t r = 0; r < PENDANT_MATRIX_ROWS; ++r)
    {
        // Set all rows HIGH, then drive this one LOW
        for (size_t rr = 0; rr < PENDANT_MATRIX_ROWS; ++rr)
            digitalWrite(PENDANT_ROW_PINS[rr], rr == r ? LOW : HIGH);
        delayMicroseconds(20);

        // Read each column
        for (size_t c = 0; c < PENDANT_MATRIX_COLS; ++c)
        {
            bool raw = (digitalRead(PENDANT_COL_PINS[c]) == LOW);
            KeyInfo &ki = key_matrix[r][c];

            // Debounce edge detection
            if (raw != ki.last_raw)
            {
                ki.last_raw = raw;
                ki.last_change_ms = millis();
            }
            else if (millis() - ki.last_change_ms >= KEY_DEBOUNCE_MS)
            {
                uint8_t bitIndex = r * PENDANT_MATRIX_COLS + c;

                if (raw && ki.state == KeyState::IDLE)
                {
                    ki.state = KeyState::PRESSED;
                    data_changed_flag = true;
                    if (bitIndex < 32)
                        current_button_bitmask |= (1u << bitIndex);
                }
                else if (!raw && (ki.state == KeyState::PRESSED || ki.state == KeyState::HELD))
                {
                    ki.state = KeyState::RELEASED;
                    data_changed_flag = true;
                    if (bitIndex < 32)
                        current_button_bitmask &= ~(1u << bitIndex);
                }
                else if (ki.state == KeyState::PRESSED)
                {
                    ki.state = KeyState::HELD;
                }
            }
        }
    }

    // Return all rows to HIGH
    for (size_t rr = 0; rr < PENDANT_MATRIX_ROWS; ++rr)
        digitalWrite(PENDANT_ROW_PINS[rr], HIGH);
#endif
}

static void read_encoders()
{
    int32_t diff = get_handwheel_diff();
    if (diff != 0)
    {
        handwheel_position += diff;
        data_changed_flag = true;
    }
}

static void read_selectors()
{
#if defined(AXIS_SELECTOR_ADC)
    uint32_t rawA = analogRead(AXIS_SELECTOR_ADC);
    uint8_t maxAxes = pendant_web_cfg.num_dro_axes;
    uint8_t newAxis = (rawA * maxAxes) / 4096;
    if (newAxis != selected_axis)
    {
        selected_axis = newAxis;
        data_changed_flag = true;
    }
#endif

#if defined(STEP_SELECTOR_ADC)
    uint32_t rawS = analogRead(STEP_SELECTOR_ADC);
    constexpr uint8_t NUM_STEPS = 4;
    uint8_t newStep = (rawS * NUM_STEPS) / 4096;
    if (newStep != selected_step)
    {
        selected_step = newStep;
        data_changed_flag = true;
    }
#endif
}
