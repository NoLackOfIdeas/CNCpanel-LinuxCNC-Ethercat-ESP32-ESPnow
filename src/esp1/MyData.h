#ifndef CUSTOM_PDO_NAME_H
#define CUSTOM_PDO_NAME_H

//-------------------------------------------------------------------//
//                                                                   //
//     This file has been created by the Easy Configurator tool      //
//                                                                   //
//     Easy Configurator project MyData.prj
//                                                                   //
//-------------------------------------------------------------------//

#define CUST_BYTE_NUM_OUT 60
#define CUST_BYTE_NUM_IN 103
#define TOT_BYTE_NUM_ROUND_OUT 60
#define TOT_BYTE_NUM_ROUND_IN 104

typedef union //---- output buffer ----
{
	uint8_t Byte[TOT_BYTE_NUM_ROUND_OUT];
	struct
	{
		// --- Standard HMI Feedback ---
		uint8_t led_matrix[8];	   // 64 LED states (8 bytes) for the main panel
		uint32_t lcnc_status_word; // A general-purpose 32-bit status word from LinuxCNC
		float current_feedrate;	   // Current machine feedrate value for display

		// --- Extended Machine & Spindle Status ---
		uint16_t machine_status;		 // Bitmask for machine states (is_on, mode, etc.)
		uint16_t spindle_coolant_status; // Bitmask for spindle/coolant states

		// --- Override Status ---
		float feed_override;	// Current feed override percentage (e.g., 1.0 for 100%)
		float rapid_override;	// Current rapid override percentage
		float spindle_override; // Current spindle override percentage

		// --- Tool Information ---
		float current_tool_diameter; // Diameter of the active tool for cutting speed display

		// --- NEW: Absolute DRO Positions ---
		float dro_pos[6]; // Absolute positions for X,Y,Z,A,B,C axes

	} Cust;
} PROCBUFFER_OUT;

typedef union //---- input buffer ----
{
	uint8_t Byte[TOT_BYTE_NUM_ROUND_IN];
	struct
	{
		// --- ESP1 Peripherals ---
		int32_t enc_pos[8];	  // Position of up to 8 encoders on ESP1
		uint32_t spindle_rpm; // Calculated spindle RPM from Hall sensor
		uint8_t probe_states; // Bitmask for up to 8 probes on ESP1

		// --- ESP2 (Main Panel) Peripherals ---
		uint8_t button_matrix[8]; // 64 button states (8 bytes)
		int16_t joystick_axes[6]; // 6 analog axes from up to 2 joysticks
		int32_t hmi_enc_pos[8];	  // Position of up to 8 encoders on ESP2
		uint8_t rotary_pos[4];	  // Position of up to 4 rotary switches

		// --- NEW: ESP3 (Handheld Pendant) Peripherals ---
		int32_t pendant_handwheel_pos;	// Current count from the handwheel encoder
		uint32_t pendant_button_states; // Bitmask for up to 25 pendant buttons
		uint8_t pendant_selected_axis;	// Current position of the axis selector (0-5)
		uint8_t pendant_selected_step;	// Current position of the step selector (0-3)
	} Cust;
} PROCBUFFER_IN;

#endif