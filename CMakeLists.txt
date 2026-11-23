cmake_minimum_required(VERSION 3.20)
project(benchrun LANGUAGES CXX)

option(BENCHRUN_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_executable(benchrun
    main.cpp
    cli.cpp
    runner.cpp
    stats.cpp
    reporter.cpp
)

target_compile_features(benchrun PRIVATE cxx_std_20)
target_compile_options(benchrun PRIVATE -Wall -Wextra -Wpedantic)
if(BENCHRUN_ENABLE_SANITIZERS)
    target_compile_options(benchrun PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(benchrun PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME benchrun_smoke COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.sh
         ${CMAKE_CURRENT_BINARY_DIR}/benchrun)
