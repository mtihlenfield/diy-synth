cmake_minimum_required(VERSION 3.13)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(pico-sdk/pico_sdk_init.cmake)

project(cv-utils)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(cv-utils
  src/cv-utils.c
)

target_include_directories(cv-utils PRIVATE include)

target_link_libraries(cv-utils
    pico_stdlib
    hardware_spi
)


pico_add_extra_outputs(cv-utils)
