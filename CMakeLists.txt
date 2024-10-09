cmake_minimum_required(VERSION 3.12)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

project("rasterrage VGA RP2040" C ASM)

project(pico_playground C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Define an option to select the demo
option(DEMO "Select the demo to build (e.g., BOUNCING_BALL, PRIMITIVES, CUBE_SPRITE...)" BOUNCING_BALL)

if (TARGET pico_scanvideo_dpi)
    if(${DEMO} STREQUAL "BOUNCING_BALL")
        add_executable(rasterrage
            framebuffer.c
            primitive.c
            rasterrage.c
            bouncing_ball.c
        )
    elseif(${DEMO} STREQUAL "PRIMITIVES")
        add_executable(rasterrage
            framebuffer.c
            primitive.c
            rasterrage.c
            primitive_test.c
        )
    elseif(${DEMO} STREQUAL "CUBE_SPRITE")
        add_executable(rasterrage
            framebuffer.c
            primitive.c
            rasterrage.c
            cube_sprite.c
        )
    elseif(${DEMO} STREQUAL "3D")
        add_executable(rasterrage
            framebuffer.c
            primitive.c
            rasterrage.c
            raster.c
        )
    elseif(${DEMO} STREQUAL "SONIC")
        add_executable(rasterrage
            framebuffer.c
            primitive.c
            rasterrage.c
            sonic.c
        )
    else()
        message(FATAL_ERROR "Unknown demo selected: ${DEMO}")
    endif()

    target_compile_definitions(rasterrage PRIVATE
                PICO_SCANVIDEO_48MHZ=0
                PICO_USE_SW_SPIN_LOCKS=0
                )

    target_link_libraries(rasterrage PRIVATE
            pico_multicore
            pico_stdlib
            pico_scanvideo_dpi)

    pico_add_extra_outputs(rasterrage)

    pico_enable_stdio_uart(rasterrage 1)
    pico_enable_stdio_usb(rasterrage 1)
endif ()
