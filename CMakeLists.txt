# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(alligator C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
set(SOURCES alligator.cpp usb_descriptors.c tusb_config.h midi.h midi.cpp)
add_executable(alligator ${SOURCES})

pico_set_program_name(alligator "alligator")
pico_set_program_version(alligator "0.1")

# Generate PIO headers
pico_generate_pio_header(alligator ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
pico_generate_pio_header(alligator ${CMAKE_CURRENT_LIST_DIR}/uart_tx.pio)
pico_generate_pio_header(alligator ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(alligator 0)
pico_enable_stdio_usb(alligator 1)

# Add the standard library to the build
target_link_libraries(alligator
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(alligator PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(alligator 
    pico_stdlib 
    hardware_pio
    hardware_pwm
    tinyusb_device
    tinyusb_board
    )

pico_add_extra_outputs(alligator)


add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)


