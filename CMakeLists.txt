# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and supported languages
project(cmasm LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically grab source files from c, cpp, and h folders
file(GLOB SOURCES
    src/c/*.c
    src/cpp/*.cpp
    src/h/*.h
)

# Add executable target
add_executable(cmasm ${SOURCES})

# Include directories
target_include_directories(cmasm PRIVATE include)

# Add shared library target
add_library(cmasm_shared SHARED ${SOURCES})

# Include directories for the shared library
target_include_directories(cmasm_shared PRIVATE include)

# Set the output directory for the shared library
set_target_properties(cmasm_shared PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
)

# Set the output directory for the executable
set_target_properties(cmasm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
)
