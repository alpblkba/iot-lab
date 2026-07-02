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
set(CMAKE_PROJECT_NAME task3)

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
    Core/Src/iot_kernel.c
    Drivers/BSP/B-U585I-IOT02A/b_u585i_iot02a.c
    Drivers/BSP/B-U585I-IOT02A/b_u585i_iot02a_audio.c
    Drivers/BSP/B-U585I-IOT02A/b_u585i_iot02a_bus.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Drivers/BSP/B-U585I-IOT02A
    Drivers/BSP/Components/Common
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

# User added for Lab 4 Task 2 naive DFT math functions
target_link_libraries(${CMAKE_PROJECT_NAME} m)

# User added for Lab 4 Task 2 CMSIS-DSP FFT
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    ARM_MATH_CM33
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ../../external/CMSIS-DSP/Include
    ../../external/CMSIS-DSP/PrivateInclude
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ../../external/CMSIS-DSP/Source/TransformFunctions/arm_rfft_fast_f32.c
    ../../external/CMSIS-DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.c
    ../../external/CMSIS-DSP/Source/TransformFunctions/arm_cfft_f32.c
    ../../external/CMSIS-DSP/Source/TransformFunctions/arm_cfft_init_f32.c
    ../../external/CMSIS-DSP/Source/TransformFunctions/arm_cfft_radix8_f32.c
    ../../external/CMSIS-DSP/Source/TransformFunctions/arm_bitreversal.c
    ../../external/CMSIS-DSP/Source/TransformFunctions/arm_bitreversal2.c
    ../../external/CMSIS-DSP/Source/ComplexMathFunctions/arm_cmplx_mag_f32.c
    ../../external/CMSIS-DSP/Source/CommonTables/arm_common_tables.c
    ../../external/CMSIS-DSP/Source/CommonTables/arm_const_structs.c
)
