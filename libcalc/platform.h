#pragma once

//-------------------------------------------------------------------------------------------------

// MLN_TARGET_xxx is the target hardware platform - PC, PICO, etc

//-------------------------------------------------------------------------------------------------

#ifdef PICO_TARGET_NAME

#define MLN_TARGET_PICO 1

#else   // other platforms may be available...

#define MLN_TARGET_PC 1

#endif

//-------------------------------------------------------------------------------------------------

