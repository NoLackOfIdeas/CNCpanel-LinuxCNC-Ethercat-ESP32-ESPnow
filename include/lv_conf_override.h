#pragma once

// 1) If LVGL or you already defined it, wipe it out
#ifdef LV_EXPORT_CONST_INT
#undef LV_EXPORT_CONST_INT
#endif

// 2) Make all calls vanish, regardless of how many args
#define LV_EXPORT_CONST_INT(...)
