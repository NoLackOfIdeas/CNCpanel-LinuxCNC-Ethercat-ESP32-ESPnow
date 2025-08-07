// #if 1
#ifndef LV_CONF_H
#define LV_CONF_H

// #include "alias_macros.h"

/*====================
   ASSERTION SETTINGS
 *====================*/
#define LV_USE_ASSERT 0

/*========================
   COLOR AND RESOLUTION
 *========================*/
#define LV_COLOR_DEPTH 16
#define LV_COLOR_SCREEN_TRANSP 1
#define LV_HOR_RES_MAX 480
#define LV_VER_RES_MAX 320
#define LV_DPI_DEF 130

/*====================
   MEMORY SETTINGS
 *====================*/
#define LV_MEM_SIZE (32U * 1024U)
#define LV_MEM_POOL_EXPAND_SIZE 0

/*========================
   STD LIB WRAPPER
 *========================*/
#define LV_STDLIB_INCLUDE <stdlib.h>
#define LV_STDLIB_STRING_INCLUDE <string.h>
#define LV_STDLIB_PRINTF_INCLUDE <stdio.h>

#define LV_USE_STDLIB_MALLOC 1
#define LV_USE_STDLIB_FREE 1
#define LV_USE_STDLIB_MEMCPY 1
#define LV_USE_STDLIB_MEMSET 1
#define LV_USE_STDLIB_STRLEN 1
#define LV_USE_STDLIB_SPRINTF 1
#define LV_USE_STDLIB_VSPRINTF 1

#define LV_STDINT_INCLUDE <stdint.h>
#define LV_STDBOOL_INCLUDE <stdbool.h>
#define LV_STDDEF_INCLUDE <stddef.h>
#define LV_INTTYPES_INCLUDE <inttypes.h>
#define LV_LIMITS_INCLUDE <limits.h>
#define LV_STDARG_INCLUDE <stdarg.h>

/*====================
   FEATURE USAGE
 *====================*/
#define LV_USE_ANIMATION 1
#define LV_USE_TRANSFORM 1
#define LV_USE_FLEX 1
#define LV_USE_GRID 1
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_SYMBOL 1

#define LV_USE_LABEL 1
#define LV_USE_LIST 1
#define LV_USE_TEXTAREA 1

#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_40 1

/*====================
   EXTERNAL MODULES
 *====================*/
#define LV_USE_LZ4 0
#define LV_USE_API_EXTENSION 0

/*------------------------------------------------------------------------------
 * Export integer constants so tools (linker maps, doxygen, etc.) can see them
 *----------------------------------------------------------------------------*/
/*#ifndef LV_EXPORT_CONST_INT
#define LV_EXPORT_CONST_INT(name) enum \
{                                      \
    name = (name)                      \
} */

// #define LV_EXPORT_CONST_INT(name) /* nothing */

#ifdef LV_EXPORT_CONST_INT
#undef LV_EXPORT_CONST_INT
#endif

#define LV_EXPORT_CONST_INT(name) enum \
{                                      \
    name##_INT = name                  \
}

#endif /* LV_CONF_H */
// #endif /* End of content enable */
