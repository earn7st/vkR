// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "vkR_types.h"
#include "vkR_descriptors.h"

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call functors
		}

		deletors.clear();
	}
};

struct AllocatedImage {
	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
	VkExtent3D imageExtent;
	VkFormat imageFormat;
};

struct FrameData {
// Command
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
// Synchronization
	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;

	DeletionQueue _deletionQueue;
};
constexpr unsigned int FRAME_OVERLAP = 2;

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
	const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
};

class VulkanEngine {
public:

// Window and States
	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1440 , 1080 };

	struct SDL_Window* _window{ nullptr };

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

// Descriptors
	DescriptorAllocator globalDescriptorAllocator;

	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

// Pipelines
	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;

	VkPipeline _trianglePipeline;
	VkPipelineLayout _trianglePipelineLayout;

// Immediate Submits
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

// ComputeEffets: [PipelineLayout, Pipeline, ComputePushConstants]
	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{ 0 };


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
	void init_commands();
	void init_sync_structures();

	void init_descriptors();

	void init_pipelines();
	void init_background_pipelines();
	void init_triangle_pipeline();
	
	void init_imgui();
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

	void create_swapchain();
	void create_swapchain_vkb(uint32_t width, uint32_t height);

	void draw_background(VkCommandBuffer cmd);
	void draw_geometry(VkCommandBuffer cmd);

	void destroy_swapchain();

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	
};
