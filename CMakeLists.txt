# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and supported languages
project(MyProject LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/c/extmain.c
    src/cpp/helper.cpp
)

# Add executable target
add_executable(MyProject ${SOURCES})

# Include directories
target_include_directories(MyProject PRIVATE include)