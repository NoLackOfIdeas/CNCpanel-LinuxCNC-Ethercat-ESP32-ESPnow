# Auto-Generated EtherCAT Data Mapping

This document describes the exact byte-for-byte layout of the process data based on `C:\Users\Maasen.KHV-NB-MZ37\Documents\PlatformIO\Projects\CNCpanel-LinuxCNC-Ethercat-ESP32-ESPnow\CNCpanel-LinuxCNC-Ethercat-ESP32-ESPnow\src\esp1\MyData.h`.

| Direction | Byte Offset | Size (Bytes) | C++ Type | Variable Name | Description |
|:---|:---|:---|:---|:---|:---|
| IN | 0 | 32 | `int32_t[8]` | `enc_pos` | Position of up to 8 encoders on ESP1 |
| IN | 32 | 4 | `uint32_t` | `spindle_rpm` | Calculated spindle RPM from Hall sensor |
| IN | 36 | 1 | `uint8_t` | `probe_states` | Bitmask for up to 8 probes on ESP1 |
| IN | 37 | 8 | `uint8_t[8]` | `button_matrix` | 64 button states (8 bytes) |
| IN | 45 | 12 | `int16_t[6]` | `joystick_axes` | 6 analog axes from up to 2 joysticks |
| IN | 57 | 32 | `int32_t[8]` | `hmi_enc_pos` | Position of up to 8 encoders on ESP2 |
| IN | 89 | 4 | `uint8_t[4]` | `rotary_pos` | Position of up to 4 rotary switches |
| IN | 93 | 4 | `int32_t` | `pendant_handwheel_pos` | Current count from the handwheel encoder |
| IN | 97 | 4 | `uint32_t` | `pendant_button_states` | Bitmask for up to 25 pendant buttons |
| IN | 101 | 1 | `uint8_t` | `pendant_selected_axis` | Current position of the axis selector (0-5) |
| IN | 102 | 1 | `uint8_t` | `pendant_selected_step` | Current position of the step selector (0-3) |
| OUT | 0 | 8 | `uint8_t[8]` | `led_matrix` | 64 LED states (8 bytes) for the main panel |
| OUT | 8 | 4 | `uint32_t` | `lcnc_status_word` | A general-purpose 32-bit status word from LinuxCNC |
| OUT | 12 | 4 | `float` | `current_feedrate` | Current machine feedrate value for display |
| OUT | 16 | 2 | `uint16_t` | `machine_status` | Bitmask for machine states (is_on, mode, etc.) |
| OUT | 18 | 2 | `uint16_t` | `spindle_coolant_status` | Bitmask for spindle/coolant states |
| OUT | 20 | 4 | `float` | `feed_override` | Current feed override percentage (e.g., 1.0 for 100%) |
| OUT | 24 | 4 | `float` | `rapid_override` | Current rapid override percentage |
| OUT | 28 | 4 | `float` | `spindle_override` | Current spindle override percentage |
| OUT | 32 | 4 | `float` | `current_tool_diameter` | Diameter of the active tool for cutting speed display |
| OUT | 36 | 24 | `float[6]` | `dro_pos` | Absolute positions for X,Y,Z,A,B,C axes |
