#include "hmi_handler.h"
#include "config_esp2.h"
#include <Adafruit_MCP23X17.h>
#include <ESP32Encoder.h>
#include <SPI.h>

// --- Konstanten ---
#define MUX_DELAY_MS 2 // Zeit pro Spalte für LED-Multiplexing

// --- MCP-Objekte ---
Adafruit_MCP23X17 mcp_buttons;
Adafruit_MCP23X17 mcp_led_rows;
Adafruit_MCP23X17 mcp_led_cols;
Adafruit_MCP23X17 mcp_rot_switches;

// --- Encoder-Objekte ---
ESP32Encoder encoders_esp2;

// --- Lokale Zustandsvariablen ---
uint64_t current_button_states = 0;
uint64_t previous_button_states = 0;
uint64_t current_led_states = 0;
int32_t current_encoder_values = {0};
uint16_t current_poti_values = {0};
uint8_t current_rotary_switch_states = 0;

// --- Hilfsvariablen ---
bool data_changed_flag = false;
unsigned long last_mux_time = 0;
uint8_t active_led_column = 0;

// --- Private Funktionsprototypen ---
void scan_button_matrix();
void update_led_matrix();
void read_encoders();
void read_potis();

// --- Implementierung ---

void hmi_init()
{
    pinMode(PIN_LEVEL_SHIFTER_OE, OUTPUT);
    digitalWrite(PIN_LEVEL_SHIFTER_OE, HIGH);

    SPI.begin(PIN_MCP_SCK, PIN_MCP_MISO, PIN_MCP_MOSI);

    if (!mcp_buttons.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_BUTTONS))
    {
        if (DEBUG_ENABLED)
            Serial.println("Fehler MCP Tasten.");
        while (1)
            ;
    }
    if (!mcp_led_rows.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_LED_ROWS))
    {
        if (DEBUG_ENABLED)
            Serial.println("Fehler MCP LED-Zeilen.");
        while (1)
            ;
    }
    if (!mcp_led_cols.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_LED_COLS))
    {
        if (DEBUG_ENABLED)
            Serial.println("Fehler MCP LED-Spalten.");
        while (1)
            ;
    }
    if (!mcp_rot_switches.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_ROT_SWITCHES))
    {
        if (DEBUG_ENABLED)
            Serial.println("Fehler MCP Drehschalter.");
        while (1)
            ;
    }
    if (DEBUG_ENABLED)
        Serial.println("Alle MCP23S17 erfolgreich initialisiert.");

    for (int i = 0; i < MATRIX_ROWS; i++)
    {
        mcp_buttons.pinMode(BUTTON_ROW_PINS[i], OUTPUT);
        mcp_buttons.digitalWrite(BUTTON_ROW_PINS[i], HIGH);
    }
    for (int i = 0; i < MATRIX_COLS; i++)
        mcp_buttons.pinMode(BUTTON_COL_PINS[i], INPUT_PULLUP);
    for (int i = 0; i < MATRIX_ROWS; i++)
        mcp_led_rows.pinMode(LED_ROW_PINS[i], OUTPUT);
    for (int i = 0; i < MATRIX_COLS; i++)
    {
        mcp_led_cols.pinMode(LED_COL_PINS[i], OUTPUT);
        mcp_led_cols.digitalWrite(LED_COL_PINS[i], HIGH);
    }

    for (int i = 0; i < MAX_ENCODERS_ESP2; i++)
    {
        ESP32Encoder::useInternalWeakPullResistors = UP;
        encoders_esp2[i].attachHalfQuad(ENC2_A_PINS[i], ENC2_B_PINS[i]);
        encoders_esp2[i].setFilter(ENC2_GLITCH_FILTER);
        encoders_esp2[i].clearCount();
    }
}

void hmi_task()
{
    scan_button_matrix();
    read_encoders();
    read_potis();
    update_led_matrix();
}

void scan_button_matrix()
{
    previous_button_states = current_button_states;
    current_button_states = 0;
    for (int r = 0; r < MATRIX_ROWS; r++)
    {
        mcp_buttons.digitalWrite(BUTTON_ROW_PINS[r], LOW);
        for (int c = 0; c < MATRIX_COLS; c++)
        {
            if (mcp_buttons.digitalRead(BUTTON_COL_PINS[c]) == LOW)
            {
                current_button_states |= (1ULL << (r * MATRIX_COLS + c));
            }
        }
        mcp_buttons.digitalWrite(BUTTON_ROW_PINS[r], HIGH);
    }
    if (current_button_states != previous_button_states)
        data_changed_flag = true;
}

void update_led_matrix()
{
    if (millis() - last_mux_time >= MUX_DELAY_MS)
    {
        last_mux_time = millis();
        mcp_led_cols.writeGPIO(0xFFFF);
        active_led_column = (active_led_column + 1) % MATRIX_COLS;
        uint8_t row_pattern = 0;
        for (int r = 0; r < MATRIX_ROWS; r++)
        {
            if ((current_led_states >> (r * MATRIX_COLS + active_led_column)) & 1ULL)
            {
                row_pattern |= (1 << r);
            }
        }
        mcp_led_rows.writeGPIOAB(row_pattern);
        mcp_led_cols.digitalWrite(LED_COL_PINS[active_led_column], LOW);
    }
}

void read_encoders()
{
    for (int i = 0; i < MAX_ENCODERS_ESP2; i++)
    {
        int32_t new_count = encoders_esp2[i].getCount();
        if (new_count != current_encoder_values[i])
        {
            current_encoder_values[i] = new_count;
            data_changed_flag = true;
        }
    }
}

void read_potis()
{
    for (int i = 0; i < MAX_POTIS; i++)
    {
        uint16_t new_val = analogRead(POTI_PINS[i]) >> 2;
        if (abs(new_val - current_poti_values[i]) > 2)
        {
            current_poti_values[i] = new_val;
            data_changed_flag = true;
        }
    }
}

uint64_t hmi_get_button_states() { return current_button_states; }
uint64_t hmi_get_previous_button_states() { return previous_button_states; }
void hmi_set_led_states(uint64_t states) { current_led_states = states; }
bool hmi_data_has_changed()
{
    bool state = data_changed_flag;
    data_changed_flag = false;
    return state;
}
void hmi_set_data_changed_flag() { data_changed_flag = true; }

void hmi_get_full_hmi_data(struct_message_to_esp1 *data)
{
    memcpy(&data->encoder_values, &current_encoder_values, sizeof(current_encoder_values));
    memcpy(&data->poti_values, &current_poti_values, sizeof(current_poti_values));
    data->rotary_switch_positions = current_rotary_switch_states;
}