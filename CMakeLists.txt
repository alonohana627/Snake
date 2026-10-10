cmake_minimum_required(VERSION 3.28)

project(Snake
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Put all executables in one place: build/<preset>/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(SNAKE_BUILD_TESTS "Build the Snake test suite" OFF)

include(cmake/CompilerWarnings.cmake)
include(cmake/SnakeComponent.cmake)

add_subdirectory(deps)
add_subdirectory(src)

if(SNAKE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
