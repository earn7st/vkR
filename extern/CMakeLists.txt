#
# fmt
#
add_subdirectory(fmt)

#
# vk-bootstrap
#
add_subdirectory(vk-bootstrap)

#
# tinygltf
#
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF)
add_library(tinygltf INTERFACE)

target_include_directories(
    tinygltf
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf
)

#
# Dear ImGui
#
add_library(imgui STATIC

    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp

    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_vulkan.cpp
)

target_include_directories(
    imgui
    PUBLIC
        imgui
        imgui/backends
)

target_link_libraries(
    imgui
    PUBLIC
        SDL3::SDL3
        Vulkan::Vulkan
)
