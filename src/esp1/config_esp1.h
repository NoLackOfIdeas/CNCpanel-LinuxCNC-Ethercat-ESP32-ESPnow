#ifndef CONFIG_ESP1_H
#define CONFIG_ESP1_H

#include <Arduino.h>

// =================================================================
// DEBUGGING
// =================================================================
#define DEBUG_ENABLED true

// =================================================================
// GPIO ASSIGNMENT ESP1
// =================================================================
// SPI for EasyCAT Shield (Standard VSPI)
#define PIN_EC_MOSI 23
#define PIN_EC_MISO 19
#define PIN_EC_SCK 18
#define PIN_EC_CS 5

// Hall Sensor for Spindle RPM
#define HALL_SENSOR_PIN 27
#define HALL_SENSOR_PULLUP GPIO_PULLUP_ENABLE

// Inductive Probes
#define NUM_PROBES 8
const int PROBE_PINS = {26, 25, 14, 12, 13, 15, 2, 4};
#define PROBE_PULLUP_MODE GPIO_PULLUP_ENABLE

// Quadrature Encoders
#define NUM_ENCODERS 8
const int ENCODER_A_PINS = {34, 32, -1, -1, -1, -1, -1, -1}; // -1 for unused
const int ENCODER_B_PINS = {35, 33, -1, -1, -1, -1, -1, -1}; // -1 for unused
#define ENCODER_GLITCH_FILTER 1023                           // 0 to 1023, higher value = more filtering

// =================================================================
// TACHOMETER CONFIGURATION
// =================================================================
#define TACHO_MAGNETS_PER_REVOLUTION 2 // Number of magnets on the rotating part
#define TACHO_UPDATE_INTERVAL_MS 500   // Calculation interval for RPM in ms
#define TACHO_MIN_RPM_DISPLAY 10       // RPM values below this are shown as 0

#endif // CONFIG_ESP1_H