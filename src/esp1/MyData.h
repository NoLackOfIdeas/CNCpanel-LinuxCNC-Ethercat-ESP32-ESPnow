#ifndef CUSTOM_PDO_NAME_H
#define CUSTOM_PDO_NAME_H

//-------------------------------------------------------------------//
//                                                                   //
//     This file has been created by the Easy Configurator tool      //
//                                                                   //
//     Easy Configurator project MyData.prj
//                                                                   //
//-------------------------------------------------------------------//

#define CUST_BYTE_NUM_OUT 32
#define CUST_BYTE_NUM_IN 93
#define TOT_BYTE_NUM_ROUND_OUT 32
#define TOT_BYTE_NUM_ROUND_IN 94

typedef union //---- output buffer ----
{
	uint8_t Byte[TOT_BYTE_NUM_ROUND_OUT];
	struct
	{
		uint8_t led_matrix[8];			 // 64 LED states (8 bytes)
		uint32_t lcnc_status_word;		 // A 32-bit status word from LinuxCNC
		float current_feedrate;			 // Current feedrate value for display
		uint16_t machine_status;		 // Bitmask for machine states (is_on, mode, etc.)
		uint16_t spindle_coolant_status; // Bitmask for spindle/coolant states
		float feed_override;			 // Current feed override percentage (e.g., 1.0 for 100%)
		float rapid_override;			 // Current rapid override percentage
		float spindle_override;			 // Current spindle override percentage

	} Cust;
} PROCBUFFER_OUT;

typedef union //---- input buffer ----
{
	uint8_t Byte[TOT_BYTE_NUM_ROUND_IN];
	struct
	{
		int32_t enc_pos[8];		  // Position of up to 8 encoders on ESP1
		uint32_t spindle_rpm;	  // Calculated spindle RPM from Hall sensor
		uint8_t probe_states;	  // Bitmask for up to 8 probes on ESP1
		uint8_t button_matrix[8]; // 64 button states (8 bytes)
		int16_t joystick_axes[6]; // 6 analog axes from up to 2 joysticks
		int32_t hmi_enc_pos[8];	  // Position of up to 8 encoders on ESP2
		uint8_t rotary_pos[4];	  // Position of up to 4 rotary switches
	} Cust;
} PROCBUFFER_IN;

#endif