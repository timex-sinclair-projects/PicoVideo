# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Locate the Pico SDK: pass -DPICO_SDK_PATH=... or set the PICO_SDK_PATH
# environment variable. Released firmware was built with Pico SDK 1.5.1.
if(NOT DEFINED PICO_SDK_PATH)
    if(DEFINED ENV{PICO_SDK_PATH})
        set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    else()
        message(FATAL_ERROR "Set PICO_SDK_PATH to your pico-sdk checkout")
    endif()
endif()

# Pull in Raspberry Pi Pico SDK (must be before project)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)


# Set the name of the project (as PROJECT_NAME) and C/C++ standards
project(picoTsVid C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# List source files for the project here
add_executable(picoTsVid
    picoTsVid.c
    globals.c

    coreB/busMain.c

    coreV/vidMain.c
    coreV/vgaPioSm.pio
    coreV/displayProc80.c
#    coreV/displayProc.c
#    coreV/col80.c
)

pico_generate_pio_header(picoTsVid ${CMAKE_CURRENT_LIST_DIR}/coreV/vgaPioSm.pio)

# Link to the library functions (stdlib, etc)
target_link_libraries(picoTsVid
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_irq
    pico_multicore
    )

pico_enable_stdio_usb(picoTsVid 1)
pico_enable_stdio_uart(picoTsVid 0)

# Create the output file
pico_add_extra_outputs(picoTsVid)
