/**
 * @file config_esp1.h
 * @brief Hardware configuration for ESP1 (The EtherCAT Real-Time Bridge).
 *
 * This file defines all physical pin connections and static parameters
 * for peripherals directly connected to the ESP1 microcontroller.
 * These values are considered the "single source of truth" for the
 * physical layout of the real-time communication bridge.
 */

#ifndef CONFIG_ESP1_H
#define CONFIG_ESP1_H
#include <Arduino.h>

// --- GENERAL SETTINGS ---
#define DEBUG_ENABLED true // Set to true to enable serial debug output, false to disable.

// --- ETHERCAT SHIELD PINS ---
// These are the standard VSPI pins used by the EasyCAT PRO shield.
// Do not change these unless you have custom hardware.
#define PIN_EC_MOSI 23
#define PIN_EC_MISO 19
#define PIN_EC_SCK 18
#define PIN_EC_CS 5

// --- HIGH-SPEED SENSOR PINS ---
// Hall sensor for spindle speed (RPM) calculation.
#define HALL_SENSOR_PIN 27

// Inductive probes for homing or probing tasks.
#define NUM_PROBES 4                                 // The number of probes connected to ESP1.
const int PROBE_PINS[NUM_PROBES] = {26, 25, 14, 12}; // The GPIO pins for each probe.

// Quadrature encoders for position feedback.
#define NUM_ENCODERS 2                             // The number of encoders connected to ESP1.
const int ENCODER_A_PINS[NUM_ENCODERS] = {34, 32}; // GPIO pins for Channel A of each encoder.
const int ENCODER_B_PINS[NUM_ENCODERS] = {35, 33}; // GPIO pins for Channel B of each encoder.

// --- TACHOMETER CONFIGURATION ---
// Use this section to define the spindle speed sensor.

// Step 1: Choose the sensor type. Options are HALL_SENSOR or ENCODER.
#define SPINDLE_SENSOR_TYPE ENCODER

// Step 2: Configure the parameters for your chosen sensor type.

#if SPINDLE_SENSOR_TYPE == HALL_SENSOR
                                                   // -- Configuration for Hall Sensor --
#define HALL_SENSOR_PIN 27
#define TACHO_MAGNETS_PER_REVOLUTION 2

#elif SPINDLE_SENSOR_TYPE == ENCODER
                                                   // -- Configuration for Quadrature Encoder --
// Note: This uses one of the available encoder slots from the main encoder config.
#define SPINDLE_ENCODER_INDEX 0  // Use Encoder #0 for the spindle
#define SPINDLE_ENCODER_PPR 1024 // Pulses Per Revolution of your spindle encoder
#endif

// The interval in milliseconds at which to calculate a new RPM value.
#define TACHO_UPDATE_INTERVAL_MS 500

#endif // CONFIG_ESP1_H