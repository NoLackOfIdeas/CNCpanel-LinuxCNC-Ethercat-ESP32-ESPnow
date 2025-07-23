#ifndef SHARED_STRUCTURES_H
#define SHARED_STRUCTURES_H

#include <stdint.h>

// --- Konfiguration der maximalen Peripherieanzahl ---
// Diese Werte müssen mit den Werten in config_esp2.h übereinstimmen
#define MAX_ENCODERS_ESP2 2
#define MAX_POTIS 6

// --- MAC-Adressen ---
// TRAGEN SIE HIER DIE MAC-ADRESSE IHRES ESP1 EIN
static uint8_t esp1_mac_address = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// TRAGEN SIE HIER DIE MAC-ADRESSE IHRES ESP2 EIN
static uint8_t esp2_mac_address = {0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0xEF};

// Datenstruktur, die von ESP2 (HMI) an ESP1 (Bridge) gesendet wird
typedef struct struct_message_to_esp1
{
    uint64_t logical_button_states; // 64 Tasten als 64-Bit-Bitmaske (logischer Zustand nach Toggle/Radio)
    int32_t encoder_values;
    uint16_t poti_values;
    uint8_t rotary_switch_positions;
} struct_message_to_esp1;

// Datenstruktur, die von ESP1 (Bridge) an ESP2 (HMI) gesendet wird
typedef struct struct_message_to_esp2
{
    uint64_t led_states_from_lcnc; // 64 LEDs als 64-Bit-Bitmaske
    uint32_t linuxcnc_status;
    // Weitere Daten von LinuxCNC für die Anzeige auf der HMI
} struct_message_to_esp2;

#endif // SHARED_STRUCTURES_H