add_executable(vkR
    vkR.cpp
    vkR_types.h
    vkR_initializers.cpp
    vkR_initializers.h
    vkR_images.h
    vkR_images.cpp 
    vkR_descriptors.h
    vkR_descriptors.cpp
    vkR_pipelines.h
    vkR_pipelines.cpp
    vkR_engine.h
    vkR_engine.cpp
    vkR_loader.h
    vkR_loader.cpp
    camera.cpp
    camera.h
)

target_include_directories(
    vkR
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/extern
)

target_link_libraries(
    vkR
    PRIVATE
        Vulkan::Vulkan
        fmt::fmt
        vk-bootstrap
        imgui
        fastgltf::fastgltf
)

target_compile_definitions(vkR
    PRIVATE
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)

add_dependencies(
    vkR
    Shaders
)