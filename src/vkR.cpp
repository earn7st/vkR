#include <iostream>
#include <cstdlib>
#include <assert.h>
#include <vector>
#include <array>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#if 1
#include <vulkan/vulkan.h>
#else
#include <vulkan/vulkan.hpp>
#endif

#define VK_CHECK(call)                                                      \
    do {                                                                    \
        VkResult result_ = call;                                            \
        if (result_ != VK_SUCCESS) {                                        \
            std::cerr << "[Vulkan ERROR] " << #call                          \
                      << " returned " << result_                             \
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

VkInstance CreateInstance()
{
	LOG("Initializing Vulkan Instance");

	VK_CHECK(volkInitialize());

// Extensions
	uint32_t availableInstanceExtensionCount = 0;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, nullptr));
	
	std::vector<VkExtensionProperties> availableInstanceExtensions(availableInstanceExtensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, availableInstanceExtensions.data()));

	std::vector<const char*> requiredInstanceExtensions{ VK_KHR_SURFACE_EXTENSION_NAME };
#ifdef _DEBUG
	requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
	requiredInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
	if (!ValidateExtensions(requiredInstanceExtensions, availableInstanceExtensions))
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}
	

// Layers
	std::vector<const char*> requiredInstanceLayers;

#if _DEBUG
	
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
	return instance;
}

VkSurfaceKHR CreateSurfaceSDL(SDL_Window* window, VkInstance& instance)
{
	VkSurfaceKHR surface;
	if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) 
	{
		std::cout << SDL_GetError() << std::endl;
		assert(false);
	}
	return surface;
}

VkDevice CreateDevice(VkInstance instance, VkSurfaceKHR surface)
{
	uint32_t physicalDeviceCount = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

	
	// pick the first physical device
	// TODO: Pick discrete GPU if available, and check for required features
	VkPhysicalDevice physicalDevice = physicalDevices[0];
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());


	uint32_t graphicsQueueIndex = -1;
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		VkBool32 supportsPresent;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent);

		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent)
		{
			graphicsQueueIndex = i;
			break;
		}
	}

	VkPhysicalDeviceProperties2 physicalDeviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);
	std::cout << "Selected device: " << physicalDeviceProperties.properties.deviceName << "\n";

	uint32_t deviceExtensionCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr));
	std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data()));

	// Device Extensions
	std::vector<const char*> requiredDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	// Shaders generated by Slang require a certain SPIR-V environment that can't be satisfied by Vulkan 1.0, so we need to expliclity up that to at least 1.1 and enable some required extensions
	requiredDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	requiredDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	requiredDeviceExtensions.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);

	if (!ValidateExtensions(requiredDeviceExtensions, deviceExtensions))
	{
		throw std::runtime_error("Required device extensions are missing.");
	}

	const float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCI{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueCI.queueFamilyIndex = static_cast<uint32_t>(graphicsQueueIndex),
	queueCI.queueCount = 1,
	queueCI.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo device_info{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCI,
		.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
		.ppEnabledExtensionNames = requiredDeviceExtensions.data() };

	VkDevice device;
	VK_CHECK(vkCreateDevice(physicalDevice, &device_info, nullptr, &device));
	return device;
}

int main()
{

	CHECK(SDL_Init(SDL_INIT_VIDEO));
	CHECK(SDL_Vulkan_LoadLibrary(NULL));

	SDL_Window* window = SDL_CreateWindow("vkR", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	assert(window);
	
	VkInstance instance = CreateInstance();
	assert(instance);
	volkLoadInstance(instance);

	VkSurfaceKHR surface = CreateSurfaceSDL(window, instance);
	assert(surface);

	VkDevice device = CreateDevice(instance, surface);
	assert(device);

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
