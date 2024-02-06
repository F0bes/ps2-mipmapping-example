cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE $ENV{PS2SDK}/ps2dev.cmake)

project(mipmapping-example)

add_executable(mipmapping-example.elf main.c mip_level_defines.h)

target_compile_options(mipmapping-example.elf PRIVATE
    -O3
    -Wall
    -Werror
)
set(MIP_LEVELS 0 1 2 3 4)

foreach(level ${MIP_LEVELS})
    add_custom_command(
        OUTPUT  mip_level_${level}.c
        COMMAND bin2c ${CMAKE_SOURCE_DIR}/mip_level_${level}.data mip_level_${level}.c mip_level_${level}
        DEPENDS mip_level_${level}.data
    )

    target_sources(mipmapping-example.elf PRIVATE mip_level_${level}.c)
endforeach()


target_link_libraries(mipmapping-example.elf
    PRIVATE
    kernel
    dma
    draw
    graph)

add_custom_target(run
	COMMAND ps2client execee host:${CMAKE_PROJECT_NAME}.elf
	DEPENDS ${CMAKE_PROJECT_NAME}.elf
	USES_TERMINAL
)

add_custom_target(reset
	COMMAND ps2client reset
)

add_custom_target(emu
	COMMAND $ENV{PCSX2} -elf ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf
	DEPENDS ${CMAKE_PROJECT_NAME}.elf
	USES_TERMINAL
)

