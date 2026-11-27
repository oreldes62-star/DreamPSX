cmake_minimum_required(VERSION 3.16)

project(DreamPSX C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(dreampsx_core STATIC
    src/bus/bus.c
    src/bus/cdrom.c
    src/bus/dma.c
    src/bus/irq.c
    src/bus/irq_mmio.c
    src/bus/memory.c
    src/bus/spu.c
    src/audio/spu_adpcm.c
    src/audio/audio_buffer.c
    src/audio/dreamcast_audio_stream.c
    src/cpu/cop0.c
    src/cpu/cop0_irq.c
    src/cpu/cpu.c
    src/gte/gte.c
    src/gpu/gpu.c
    src/media/iso9660.c
    src/media/disc_image.c
    src/media/xa_adpcm.c
    src/media/boot_config.c
    src/media/psx_exe.c
    src/machine/machine.c
    src/system/timers.c
)

target_include_directories(dreampsx_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dreampsx_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

enable_testing()

set(DREAMPSX_TESTS
    test_dma
    test_cdrom
    test_iso9660
    test_disc_image
    test_xa_adpcm
    test_boot_config
    test_psx_exe
    test_bus
    test_cop0_exception
    test_cop0_irq
    test_cpu_context
    test_cpu_execution
    test_cpu_integer
    test_irq
    test_irq_cop0_integration
    test_irq_mmio
    test_gte
    test_gpu
    test_gpu_texture_clip
    test_gpu_texture_wrap
    test_gpu_raw_texture
    test_gpu_texture_opcodes
    test_gpu_texture_rasterization
    test_gpu_texture_shared_edges
    test_gpu_indexed_texture_boundaries
    test_gpu_rectangles
    test_gpu_textured_rectangles
    test_gpu_framebuffer
    test_gpu_display
    test_gpu_display_mode
    test_gpu_video_timing
    test_machine_video_irq
    test_timers
    test_timers_bus
    test_timers_irq
    test_gpu_scanout
    test_machine
    test_machine_bus
    test_machine_boot
    test_memory
    test_spu
    test_audio_buffer
    test_dreamcast_audio_stream
)

foreach(test_name IN LISTS DREAMPSX_TESTS)
    add_executable(${test_name} tests/${test_name}.c)
    target_link_libraries(${test_name} PRIVATE dreampsx_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
