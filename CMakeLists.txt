cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(lcd-sandbox C CXX ASM)

pico_sdk_init()

add_executable(lcd-sandbox
    st7735.c
    main.c
)

target_link_libraries(lcd-sandbox 
    pico_stdlib
    hardware_spi
    hardware_gpio
)

# This is what makes it show up as a COM port over USB
pico_enable_stdio_usb(lcd-sandbox 1)
pico_enable_stdio_uart(lcd-sandbox 0)

pico_add_extra_outputs(lcd-sandbox)