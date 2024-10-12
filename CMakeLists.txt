cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()

include(pico_sdk_import.cmake)
project(pico_inverter)
pico_sdk_init()

add_executable(pico_inverter src/main.cpp)
pico_add_extra_outputs(pico_inverter)

pico_enable_stdio_usb(pico_inverter 1)
pico_enable_stdio_uart(pico_inverter 0)

# pico_generate_pio_header(pico_inverter ${CMAKE_CURRENT_LIST_DIR}/src/pio/inverter.pio)

target_link_libraries(pico_inverter PRIVATE
        pico_stdlib
        hardware_pwm
        hardware_dma
        hardware_irq
)
