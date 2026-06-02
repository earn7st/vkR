#include <iostream>
#include <cstdlib>
#include <assert.h>
#include <vector>
#include <array>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#if 1
#include <vulkan/vulkan.h>
#else
#include <vulkan/vulkan.hpp>
#endif

#include <glm/glm.hpp>

#define VK_CHECK(call)                                                      \
    do {                                                                    \
        VkResult result_ = call;                                            \
        if (result_ != VK_SUCCESS) {                                        \
            std::cerr << "[Vulkan ERROR] " << #call                         \
                      << " returned " << result_                            \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl;\
            std::abort();                                                   \
        }                                                                   \
    } while (0)

#define CHECK(call)                                                         \
    do {                                                                    \
        auto result_ = call;                                                \
        if (!result_) {                                                     \
            std::cerr << "[ERROR] " << #call                                \
                      << " failed at " << __FILE__ << ":" << __LINE__       \
                      << std::endl;                                         \
            std::abort();                                                   \
        }                                                                   \
    } while (0)

#define LOG(content)	\
	std::cout << "[vkR LOG] " << content << std::endl; 

struct SwapchainDimensions
{
	/// Width of the swapchain.
	uint32_t width = 0;

	/// Height of the swapchain.
	uint32_t height = 0;

	/// Pixel format of the swapchain.
	VkFormat format = VK_FORMAT_UNDEFINED;
};

struct PerFrame
{
	VkFence         queueSubmitFence = VK_NULL_HANDLE;
	VkCommandPool   primaryCommandPool = VK_NULL_HANDLE;
	VkCommandBuffer primaryCommandBuffer = VK_NULL_HANDLE;
	VkSemaphore     swapchainAcquireSemaphore = VK_NULL_HANDLE;
	VkSemaphore     swapchainReleaseSemaphore = VK_NULL_HANDLE;
};

struct Context
{
	SDL_Window* window;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	int32_t graphicsQueueIndex = -1;
	VkDevice device;
	VkSwapchainKHR swapchain;
	std::vector<VkImageView> swapchainImageViews;
	SwapchainDimensions swapchainDimensions;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	std::vector<VkSemaphore> semaphores;
	VmaAllocator vmaAllocator;

	std::vector<PerFrame> perFrame;

} context;

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
};

VkBuffer vBuffer;
VkDeviceMemory vBufferMemory = VK_NULL_HANDLE;
VmaAllocation vBufferAllocation = VK_NULL_HANDLE;

bool ValidateExtensions(const std::vector<const char*> &required, const std::vector<VkExtensionProperties> &available)
{
	for (auto extension : required)
	{
		bool found = false;
		for (auto& available_extension : available)
		{
			if (strcmp(available_extension.extensionName, extension) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found) return false;
	}

	return true;
}

VkInstance InitInstance()
{
	LOG("Initializing Vulkan Instance");

	VK_CHECK(volkInitialize());

// Extensions
	uint32_t instanceExtensionCount = 0;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));
	
	std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data()));

	std::vector<const char*> requiredInstanceExtensions{ VK_KHR_SURFACE_EXTENSION_NAME };
#ifdef VKR_DEBUG
	requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
	requiredInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
	if (!ValidateExtensions(requiredInstanceExtensions, instanceExtensions))
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}
	

// Layers
	std::vector<const char*> requiredInstanceLayers;

#if VKR_DEBUG
	
	const char* debugLayerName[] = { "VK_LAYER_KHRONOS_validation" };

	for (auto layer : debugLayerName)
	{
		requiredInstanceLayers.push_back(layer);
	}
	// TODO: check if support validation layer
#endif		

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "vkR",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.pEngineName = "vkR Engine",
		.engineVersion = VK_MAKE_VERSION(0, 1, 0),
		.apiVersion = VK_API_VERSION_1_4,
	};

	VkInstanceCreateInfo instanceCI{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(requiredInstanceLayers.size()),
		.ppEnabledLayerNames = requiredInstanceLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size()),
		.ppEnabledExtensionNames = requiredInstanceExtensions.data()
	};

	VkInstance instance;
	VK_CHECK(vkCreateInstance(&instanceCI, nullptr, &instance));
	volkLoadInstance(instance);
	return instance;
}

VkSurfaceKHR InitSurfaceSDL()
{
	VkSurfaceKHR surface;
	if (!SDL_Vulkan_CreateSurface(context.window, context.instance, nullptr, &surface))
	{
		std::cout << SDL_GetError() << std::endl;
		assert(false);
	}                                  
	return surface;
}

VkDevice InitDevice()
{
	LOG("Initializing Device");

	uint32_t physicalDeviceCount = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, nullptr));
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, physicalDevices.data()));

	// pick the first physical device
	// TODO: pick discrete GPU if available, and check for required features
	VkPhysicalDevice physicalDevice = physicalDevices[0];
	context.physicalDevice = physicalDevice;
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	uint32_t graphicsQueueIndex = -1;
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		VkBool32 supportsPresent;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, context.surface, &supportsPresent);

		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent)
		{
			graphicsQueueIndex = i;
			break;
		}
	}

	// print selected device name
	VkPhysicalDeviceProperties2 physicalDeviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);
	std::cout << "[vkR Info] Selected device: " << physicalDeviceProperties.properties.deviceName << "\n";

	uint32_t deviceExtensionCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr));
	std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data()));

	// Device Extensions
	std::vector<const char*> requiredDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	// currently using slang
	// TODO: support other shader languages
	requiredDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	requiredDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	requiredDeviceExtensions.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);

	if (!ValidateExtensions(requiredDeviceExtensions, deviceExtensions))
	{
		throw std::runtime_error("Required device extensions are missing.");
	}

	const float queuePriority = 1.0f;

	 VkDeviceQueueCreateInfo queueCI{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = static_cast<uint32_t>(graphicsQueueIndex),
		.queueCount = 1,
		.pQueuePriorities = &queuePriority };

	VkDeviceCreateInfo device_info{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCI,
		.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
		.ppEnabledExtensionNames = requiredDeviceExtensions.data() };

	VkDevice device;
	VK_CHECK(vkCreateDevice(physicalDevice, &device_info, nullptr, &device));
	volkLoadDevice(device);



	return device;
}

void InitVMA()
{
	VmaVulkanFunctions vma_vulkan_func{
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr };

	VmaAllocatorCreateInfo allocator_info{
		.physicalDevice = context.physicalDevice,
		.device = context.device,
		.pVulkanFunctions = &vma_vulkan_func,
		.instance = context.instance };

	VkResult result = vmaCreateAllocator(&allocator_info, &context.vmaAllocator);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create allocator for VMA allocator");
	}
	return;
}

void teardownPerFrame(PerFrame& perFrame)
{
	if (perFrame.queueSubmitFence != VK_NULL_HANDLE)
	{
		vkDestroyFence(context.device, perFrame.queueSubmitFence, nullptr);

		perFrame.queueSubmitFence = VK_NULL_HANDLE;
	}

	if (perFrame.primaryCommandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(context.device, perFrame.primaryCommandPool, 1, &perFrame.primaryCommandBuffer);

		perFrame.primaryCommandBuffer = VK_NULL_HANDLE;
	}

	if (perFrame.primaryCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(context.device, perFrame.primaryCommandPool, nullptr);

		perFrame.primaryCommandPool = VK_NULL_HANDLE;
	}

	if (perFrame.swapchainAcquireSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(context.device, perFrame.swapchainAcquireSemaphore, nullptr);

		perFrame.swapchainAcquireSemaphore = VK_NULL_HANDLE;
	}

	if (perFrame.swapchainReleaseSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(context.device, perFrame.swapchainReleaseSemaphore, nullptr);

		perFrame.swapchainReleaseSemaphore = VK_NULL_HANDLE;
	}
}

void InitPerFrame(PerFrame& perFrame)
{
	VkFenceCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT };
	VK_CHECK(vkCreateFence(context.device, &info, nullptr, &perFrame.queueSubmitFence));

	VkCommandPoolCreateInfo cmdPoolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = static_cast<uint32_t>(context.graphicsQueueIndex) };
	VK_CHECK(vkCreateCommandPool(context.device, &cmdPoolInfo, nullptr, &perFrame.primaryCommandPool));

	VkCommandBufferAllocateInfo cmdBufInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = perFrame.primaryCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1 };
	VK_CHECK(vkAllocateCommandBuffers(context.device, &cmdBufInfo, &perFrame.primaryCommandBuffer));
}

void InitSwapchain()
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &surfaceCapabilities));

	VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;

	VkExtent2D swapchainSize{};
	if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
	{
		swapchainSize.width = 1920;
		swapchainSize.height = 1080;
	}
	else
	{
		swapchainSize = surfaceCapabilities.currentExtent;
	}

	// FIFO must be supported by all implementations.
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	uint32_t desiredSwapchainImageCount = surfaceCapabilities.minImageCount + 1;
	if ((surfaceCapabilities.maxImageCount > 0) && (desiredSwapchainImageCount > surfaceCapabilities.maxImageCount))
	{
		desiredSwapchainImageCount = surfaceCapabilities.maxImageCount;
	}

	VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	VkSwapchainKHR oldSwapchain = context.swapchain;

	// using opaque alpha blending, ignoring alpha channel
	// alpha blending shouldn't happen in presenting stage
	VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VkSwapchainCreateInfoKHR info{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = context.surface,
		.minImageCount = desiredSwapchainImageCount,
		.imageFormat = format,
		.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = swapchainSize,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = preTransform,
		.compositeAlpha = composite,
		.presentMode = swapchainPresentMode,
		.clipped = true,
		.oldSwapchain = oldSwapchain };

	VK_CHECK(vkCreateSwapchainKHR(context.device, &info, nullptr, &context.swapchain));

	if (oldSwapchain != VK_NULL_HANDLE)
	{
		LOG("Old swapchain is not NULL");
	}

	context.swapchainDimensions = { swapchainSize.width, swapchainSize.height, format};

	uint32_t imageCount;
	VK_CHECK(vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, nullptr));

	std::vector<VkImage> swapchainImages(imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, swapchainImages.data()));

	// Initialize per-frame resources.
	// Every swapchain image has its own command pool and fence manager. (to be assured)
	// This makes it very easy to keep track of when we can reset command buffers and such.
	context.perFrame.clear();
	context.perFrame.resize(imageCount);

	for (size_t i = 0; i < imageCount; i++)
	{
		InitPerFrame(context.perFrame[i]);
	}

	for (size_t i = 0; i < imageCount; i++)
	{
		VkImageViewCreateInfo view_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = context.swapchainDimensions.format,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1} };

		VkImageView imageView;
		VK_CHECK(vkCreateImageView(context.device, &view_info, nullptr, &imageView));

		context.swapchainImageViews.push_back(imageView);
	}
}

void InitVertexBuffer()
{
	const std::vector<Vertex> vertices = {
		{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}} };

	const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	// Copy Vertex data to a buffer accessible by the device

	VkBufferCreateInfo bufferCI{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT };

	// We use the Vulkan Memory Allocator to find a memory type that can be written and mapped from the host
	// On most setups this will return a memory type that resides in VRAM and is accessible from the host
	VmaAllocationCreateInfo bufferACI{
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
		.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

	VmaAllocationInfo bufferAI{};
	VK_CHECK(vmaCreateBuffer(context.vmaAllocator, &bufferCI, &bufferACI, &vBuffer, &vBufferAllocation, &bufferAI));
	if (bufferAI.pMappedData)
	{
		memcpy(bufferAI.pMappedData, vertices.data(), bufferSize);
	}
	else
	{
		throw std::runtime_error("Could not map vertex buffer.");
	}
}

int main()
{

	CHECK(SDL_Init(SDL_INIT_VIDEO));
	CHECK(SDL_Vulkan_LoadLibrary(NULL));
	context.window = SDL_CreateWindow("vkR", 1920u, 1080u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	assert(context.window);
	
	context.instance = InitInstance();
	assert(context.instance);
	

	context.surface = InitSurfaceSDL();
	assert(context.surface);

	context.device = InitDevice();
	assert(context.device);

	InitVMA();
	assert(context.vmaAllocator);

	InitVertexBuffer();
	assert(vBuffer);
	assert(vBufferAllocation);

	InitSwapchain();

	bool running = true;
	while (running)
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}
		}
	}


    std::cout << "Hello World!\n";
}
