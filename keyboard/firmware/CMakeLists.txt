cmake_minimum_required(VERSION 3.13)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(pico-sdk/pico_sdk_init.cmake)

project(keyboard)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(keyboard
  src/keyboard.c
  src/mcp4921.c
  src/io.c
  src/lkp_stack.c
  include/mcp4921.h
  include/io.h
  include/list.h
  include/lkp_stack.h
  include/hardware_config.h
)

target_include_directories(keyboard PRIVATE include)

target_link_libraries(keyboard
	pico_stdlib
	pico_multicore
	hardware_spi
	hardware_adc)


pico_add_extra_outputs(keyboard)
