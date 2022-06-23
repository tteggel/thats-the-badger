project(thats-the-badger C CXX)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    main.cpp
    state.cpp
    sdc4x.cpp
)

pico_set_program_name(${PROJECT_NAME} "${PROJECT_NAME}")
pico_set_program_version(${PROJECT_NAME} "0.1")

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)


target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
    badger2040
    scd4x
    pico_multicore
)


pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_add_extra_outputs(${PROJECT_NAME})

include_directories(../pimoroni-pico)
