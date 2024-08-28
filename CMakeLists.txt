cmake_minimum_required(VERSION 3.12)

project("Vega6502 VGA RP2040")

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

project(pico_playground C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

if (TARGET pico_scanvideo_dpi)
    add_executable(vega6502
            vega6502.c
            primitive.c
            framebuffer.c
            )

    target_link_libraries(vega6502 PRIVATE
            pico_multicore
            pico_stdlib
            pico_scanvideo_dpi)

    pico_add_extra_outputs(vega6502)

    pico_enable_stdio_uart(vega6502 1)
    pico_enable_stdio_usb(vega6502 1)
endif ()
