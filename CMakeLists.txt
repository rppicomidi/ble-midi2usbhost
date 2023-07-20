cmake_minimum_required(VERSION 3.13)
set(target_name ble-midi2usbhost)
set(PICO_BOARD pico_w)
include (pico_sdk_import.cmake)
project(${target_name})
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

# Make a GATT header file from a BTstack GATT file
# Pass the target library name library type and path to the GATT input file
# To add additional directories to the gatt #import path, one "-I[custom_path]"
# argument for each custom_path to the end of the argument list.
# TODO if pull request #1445 to the pico-sdk project is approved, then
# remove this function and use pico_btstack_make_gatt_header() instead
function(my_pico_btstack_make_gatt_header TARGET_LIB TARGET_TYPE GATT_FILE)
        find_package (Python3 REQUIRED COMPONENTS Interpreter)
        get_filename_component(GATT_NAME "${GATT_FILE}" NAME_WE)
        get_filename_component(GATT_PATH "${GATT_FILE}" PATH)
        set(GATT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
        set(GATT_HEADER "${GATT_BINARY_DIR}/${GATT_NAME}.h")
        set(TARGET_GATT "${TARGET_LIB}_gatt_header")

        add_custom_target(${TARGET_GATT} DEPENDS ${GATT_HEADER})
        add_custom_command(
                OUTPUT ${GATT_HEADER}
                DEPENDS ${GATT_FILE}
                WORKING_DIRECTORY ${GATT_PATH}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${GATT_BINARY_DIR} &&
                        ${Python3_EXECUTABLE} ${PICO_SDK_PATH}/lib/btstack/tool/compile_gatt.py ${GATT_FILE} ${GATT_HEADER} ${ARGN}
                VERBATIM)
        add_dependencies(${TARGET_LIB}
                ${TARGET_GATT}
                )
        target_include_directories(${TARGET_LIB} ${TARGET_TYPE}
                ${GATT_BINARY_DIR}
                )
endfunction()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/ring_buffer_lib)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/pico-w-ble-midi-service)
add_executable(${target_name}
    ble-midi2usbhost.c
    )
target_link_libraries(${target_name}
    pico_stdlib
    ble_midi_service_lib
    pico_btstack_cyw43
    pico_cyw43_arch_none
    ring_buffer_lib
    tinyusb_host
    tinyusb_board
    )
target_include_directories(${target_name} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config, etc.
    )
pico_enable_stdio_uart(${target_name} 1)
target_link_options(${target_name} PRIVATE -Xlinker --print-memory-usage)
target_compile_options(${target_name} PRIVATE -Wall -Wextra)
my_pico_btstack_make_gatt_header(${target_name} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble-midi2usbhost.gatt" "-I${CMAKE_CURRENT_LIST_DIR}/lib/pico-w-ble-midi-service")

pico_add_extra_outputs(${target_name})

