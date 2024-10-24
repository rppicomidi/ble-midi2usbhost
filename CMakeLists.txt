cmake_minimum_required(VERSION 3.13)
set(target_name ble-midi2usbhost)
set(PICO_BOARD pico_w)
include (pico_sdk_import.cmake)
project(${target_name})
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/ring_buffer_lib)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/pico-w-ble-midi-lib)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/usb_midi_host)

add_executable(${target_name}
    ble-midi2usbhost.c
    )
target_link_libraries(${target_name}
    pico_stdlib
    ble_midi_server_lib
    pico_btstack_cyw43
    pico_cyw43_arch_none
    ring_buffer_lib
    tinyusb_host
    tinyusb_board
    usb_midi_host_app_driver
    )
target_include_directories(${target_name} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config, etc.
    )
pico_enable_stdio_uart(${target_name} 1)
target_link_options(${target_name} PRIVATE -Xlinker --print-memory-usage)
target_compile_options(${target_name} PRIVATE -Wall -Wextra)
pico_btstack_make_gatt_header(${target_name} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble-midi2usbhost.gatt" "${CMAKE_CURRENT_LIST_DIR}/lib/pico-w-ble-midi-lib")

pico_add_extra_outputs(${target_name})

