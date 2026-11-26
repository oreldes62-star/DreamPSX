cmake_minimum_required(VERSION 3.16)

project(DreamPSX C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(dreampsx_core STATIC
    src/bus/bus.c
    src/bus/irq.c
    src/bus/irq_mmio.c
    src/bus/memory.c
    src/cpu/cop0.c
    src/cpu/cop0_irq.c
    src/cpu/cpu.c
    src/machine/machine.c
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
    test_bus
    test_cop0_exception
    test_cop0_irq
    test_cpu_context
    test_cpu_execution
    test_cpu_integer
    test_irq
    test_irq_cop0_integration
    test_irq_mmio
    test_machine
    test_machine_bus
    test_memory
)

foreach(test_name IN LISTS DREAMPSX_TESTS)
    add_executable(${test_name} tests/${test_name}.c)
    target_link_libraries(${test_name} PRIVATE dreampsx_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
