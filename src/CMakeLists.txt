# =========================
# Core library
# =========================

add_library(
    shzk_runtime

    #core
    "runtime/core/Math.cpp"
    "runtime/core/Transform.cpp"

    # global
    "runtime/global/Engine.cpp"

    # import
    "runtime/import/GltfLoader.cpp"

    # input
    "runtime/input/InputSystem.cpp"

    # asset
    "runtime/asset/AssetManager.cpp"
    "runtime/asset/Model.cpp"
    "runtime/asset/Texture.cpp"
    "runtime/asset/Material.cpp"
    "runtime/asset/Shader.cpp"

    # framework
    "runtime/framework/components/TransformComponent.cpp"
    "runtime/framework/components/MeshComponent.cpp"
    "runtime/framework/components/CameraComponent.cpp"
    "runtime/framework/components/SkyBoxComponent.cpp"
    "runtime/framework/components/SkyLightComponent.cpp"
    "runtime/framework/components/DirectionalLightComponent.cpp"
    "runtime/framework/Node.cpp"
    "runtime/framework/Scene.cpp"
    
    # render
    "runtime/render/RenderSystem.cpp"
    "runtime/render/SceneRenderer.cpp"
    "runtime/render/MeshDrawCommand.cpp"

    "runtime/render/resources/Buffer.cpp"
    "runtime/render/resources/GraphicsPipelineCache.cpp"
    "runtime/render/resources/Sampler.cpp"
    "runtime/render/resources/RenderResourceManager.cpp"
    "runtime/render/resources/VertexFactory.cpp"

    "runtime/render/passes/MeshPass.cpp"
    "runtime/render/passes/MeshPassProcessor.cpp"
    "runtime/render/passes/ForwardPass.cpp"
    "runtime/render/passes/DepthPrePass.cpp"
    "runtime/render/passes/SkyPass.cpp"
    "runtime/render/passes/PostProcessPass.cpp"
    "runtime/render/passes/IBLPass.cpp"
    
    # rhi
    "runtime/rhi/RHI.cpp"
    "runtime/rhi/RHICommandList.cpp"
    "runtime/rhi/RHIResource.cpp"
    "runtime/rhi/RHIDefinitions.cpp"

    # vulkan rhi
    "runtime/rhi/vulkan/VulkanRHI.cpp"
    "runtime/rhi/vulkan/VulkanRHICommandPool.cpp"
    "runtime/rhi/vulkan/VulkanRHIFence.cpp"
    "runtime/rhi/vulkan/VulkanRHISemaphore.cpp"
    "runtime/rhi/vulkan/VulkanRHISurface.cpp"
    "runtime/rhi/vulkan/VulkanRHISwapchain.cpp"
    "runtime/rhi/vulkan/VulkanRHIQueue.cpp"
    "runtime/rhi/vulkan/VulkanRHIResource.cpp"

    # window
    "runtime/window/WindowSystem.cpp"
)

target_compile_definitions(shzk_runtime PUBLIC 
    VK_NO_PROTOTYPES
    SHZK_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/"
    SHZK_SPIRV_DIR="${CMAKE_SOURCE_DIR}/bin/shaders/")
if(WIN32)
    target_compile_definitions(shzk_runtime PUBLIC NOMINMAX)
endif()

target_include_directories(
    shzk_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/extern
)


# macOS: system-installed headers (glm, etc.) from Homebrew
if(APPLE)
    target_include_directories(shzk_runtime PUBLIC /opt/homebrew/include)
    # VMA (VulkanMemoryAllocator) triggers nullability warnings on Apple Clang
    target_compile_options(shzk_runtime PRIVATE -Wno-nullability-completeness)
endif()

target_link_libraries(
    shzk_runtime
    PUBLIC
        Vulkan::Vulkan
        fmt::fmt
        vk-bootstrap
        imgui
        fastgltf::fastgltf
        stb
        mikktspace
)

add_dependencies(
    shzk_runtime
    Shaders
)

# =========================
# Editor executable
# =========================

add_executable(
    shzk_editor
    
    "editor/Editor.cpp"
    "editor/main.cpp"
)

target_link_libraries(
    shzk_editor
    PRIVATE
        shzk_runtime
)