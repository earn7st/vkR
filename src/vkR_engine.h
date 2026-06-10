// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "vkR_types.h"
#include "vkR_descriptors.h"

struct MeshAsset;

struct FrameData {
	// Command
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
	// Synchronization
	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;

	DeletionQueue _deletionQueue;
	DescriptorAllocatorGrowable _frameDescriptors;
};
constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:

// Window and States
	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1920 , 1080 };

	struct SDL_Window* _window{ nullptr };
	bool resize_requested = false;

// Vulkan Objects
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;

// Frame Datas
	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

// Destruction Queue
	DeletionQueue _mainDeletionQueue;

// VMA Allocator
	VmaAllocator _allocator;

// Draw Image
	AllocatedImage _drawImage;
	VkExtent2D _drawExtent;
	float renderScale = 1.f;

// Depth Image
	AllocatedImage _depthImage;

// Descriptors
	DescriptorAllocator globalDescriptorAllocator;

	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

// Pipelines
	VkPipelineLayout _gradientPipelineLayout;

	VkPipelineLayout _meshPipelineLayout;
	VkPipeline _meshPipeline;

// Buffers
	GPUMeshBuffers rectangle;

// Meshes
	std::vector<std::shared_ptr<MeshAsset>> testMeshes;

// Immediate Submits
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

// ComputeEffets: [PipelineLayout, Pipeline, ComputePushConstants]
	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{ 0 };

// Scene Data using Uniform Buffer
	GPUSceneData sceneData;
	VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

// Images for Textures
	AllocatedImage _whiteImage;
	AllocatedImage _blackImage;
	AllocatedImage _greyImage;
	AllocatedImage _errorCheckerboardImage;

	VkSampler _defaultSamplerLinear;
	VkSampler _defaultSamplerNearest;

	VkDescriptorSetLayout _singleImageDescriptorLayout;


public:
	static VulkanEngine& Get();

	void init();
	void cleanup();
	void draw();
	void run();

private:
	void init_vulkan_vkb();
	void init_vulkan();
	
	void init_commands_vkb();
	void init_sync_structures();

	void init_images();
	void init_descriptors();

	void init_pipelines();
	void init_background_pipelines();
	void init_triangle_pipeline();
	void init_mesh_pipeline();

	void init_default_data();
	
	void init_imgui();
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
;
	void init_swapchain_vkb(uint32_t width, uint32_t height);
	void resize_swapchain();

	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroy_buffer(const AllocatedBuffer& buffer);
	void destroy_mesh_asset(std::shared_ptr<MeshAsset>& meshAsset);

	void draw_background(VkCommandBuffer cmd);
	void draw_geometry(VkCommandBuffer cmd);

	void destroy_swapchain();

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void destroy_image(const AllocatedImage& img);

public:
	GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
	
};

