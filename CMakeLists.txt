cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Pump_Agitator_Control)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add Application sources (C files) and include dirs
file(GLOB_RECURSE APPLICATION_SOURCES
    ${CMAKE_SOURCE_DIR}/Application/*.c
    ${CMAKE_SOURCE_DIR}/Application/*.s
)
if(APPLICATION_SOURCES)
    message(STATUS "Found Application sources: ${APPLICATION_SOURCES}")
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${APPLICATION_SOURCES})
else()
    message(STATUS "No Application sources found in ${CMAKE_SOURCE_DIR}/Application")
endif()

# Add Core C sources (Core/Src)
file(GLOB_RECURSE CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/Core/Src/*.c
    ${CMAKE_SOURCE_DIR}/Core/Src/*.s
)
list(REMOVE_ITEM CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/Core/Src/main.c
    ${CMAKE_SOURCE_DIR}/Core/Src/system_stm32f1xx.c
    ${CMAKE_SOURCE_DIR}/startup_stm32f103xb.s
)
if(CORE_SOURCES)
    message(STATUS "Found Core sources: ${CORE_SOURCES}")
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${CORE_SOURCES})
else()
    message(STATUS "No Core sources found in ${CMAKE_SOURCE_DIR}/Core/Src")
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Application
    ${CMAKE_SOURCE_DIR}/Application/Communication
    ${CMAKE_SOURCE_DIR}/Application/control
    ${CMAKE_SOURCE_DIR}/Application/system
    ${CMAKE_SOURCE_DIR}/Core/Src/Communication
    ${CMAKE_SOURCE_DIR}/Core/Src/control
    ${CMAKE_SOURCE_DIR}/Core/Src
    ${CMAKE_SOURCE_DIR}/Core/Src/system
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
