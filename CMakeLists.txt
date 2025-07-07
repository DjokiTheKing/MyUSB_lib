cmake_minimum_required(VERSION 3.13)
project(MyUSB_lib C CXX ASM)

add_library(MyUSB_lib STATIC
    ${CMAKE_CURRENT_LIST_DIR}/headers/MyUsb.h
    ${CMAKE_CURRENT_LIST_DIR}/headers/MyUSB_callbacks.h
)

target_include_directories(MyUSB_lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/headers
)

target_sources(MyUSB_lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/sources/hid_app.c
    ${CMAKE_CURRENT_LIST_DIR}/sources/MyUsb.cpp
    )

target_link_libraries(MyUSB_lib PUBLIC 
        pico_stdlib

        hardware_dma

        FreeRTOS-Kernel
        
        tinyusb_host
        tinyusb_board
)