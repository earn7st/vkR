cmake_minimum_required(VERSION 3.22)

project(vkR LANGUAGES C CXX)

#
# C++ Settings
#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_SYSTEM_VERSION 10.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#
# Packages
#
find_package(Vulkan REQUIRED)

add_library(SDL3::SDL3 SHARED IMPORTED)

set_target_properties(SDL3::SDL3 PROPERTIES

    IMPORTED_IMPLIB
        "$ENV{VULKAN_SDK}/Lib/SDL3.lib"

    IMPORTED_LOCATION
        "$ENV{VULKAN_SDK}/Bin/SDL3.dll"

    INTERFACE_INCLUDE_DIRECTORIES
        "$ENV{VULKAN_SDK}/Include"
)

#
# Output Directories
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_SOURCE_DIR}/bin)

#
# Third Party Libraries
#
add_subdirectory(extern)

#
# Engine
#
add_subdirectory(src)

#
# Shader Compilation
#
find_program(
    GLSLANG_VALIDATOR
    NAMES glslangValidator
    HINTS
        $ENV{VULKAN_SDK}/Bin
        $ENV{VULKAN_SDK}/Bin32
)

if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR
        "glslangValidator not found. Please install Vulkan SDK.")
endif()

file(MAKE_DIRECTORY
    "${PROJECT_SOURCE_DIR}/bin/shaders")

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
)

set(SPIRV_BINARY_FILES)

foreach(GLSL ${GLSL_SOURCE_FILES})

    get_filename_component(FILE_NAME ${GLSL} NAME)

    set(SPIRV
        "${PROJECT_SOURCE_DIR}/bin/shaders/${FILE_NAME}.spv"
    )

    add_custom_command(
        OUTPUT ${SPIRV}

        COMMAND
            ${GLSLANG_VALIDATOR}
            -V
            ${GLSL}
            -o
            ${SPIRV}

        DEPENDS ${GLSL}

        COMMENT "Compiling shader ${FILE_NAME}"

        VERBATIM
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})

endforeach()

add_custom_target(
    Shaders ALL
    DEPENDS ${SPIRV_BINARY_FILES}
)