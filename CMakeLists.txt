# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(molencalc C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


add_executable(molencalc
        main.c
        drivers/display.c
        drivers/display.h
        drivers/font-5x10.c
        drivers/font-10x16.c
        drivers/font.h
        drivers/keyboard.c
        drivers/keyboard.h
        drivers/lcd.c
        drivers/lcd.h
        drivers/picocalc.c
        drivers/picocalc.h
        drivers/southbridge.c
        drivers/southbridge.h
        libcalc/expr.cpp
        libcalc/funcs.cpp
        libcalc/libcalc.cpp
        libcalc/parser.cpp
        libcalc/symbols.cpp
        )

pico_set_program_name(molencalc "molencalc")
pico_set_program_version(molencalc "1.0")

set_target_properties(molencalc PROPERTIES OUTPUT_NAME "molencalc-${PICO_BOARD}")


# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(molencalc 0)
pico_enable_stdio_usb(molencalc 0)

# Turn on all warnings
target_compile_options(molencalc PRIVATE -Wall -Werror)
target_compile_options(molencalc PRIVATE -DD_PLATFORM_PICO)

# Add the standard library to the build
target_link_libraries(molencalc
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(molencalc PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(molencalc
        pico_stdlib
        pico_printf
        pico_float
        pico_status_led
        pico_rand
        hardware_gpio
        hardware_i2c
        hardware_spi
        hardware_pio
        hardware_clocks
        )

pico_add_extra_outputs(molencalc)

