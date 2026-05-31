#include <iostream>
#include <assert.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#if 1
#include <vulkan/vulkan.h>
#else
#include <vulkan/vulkan.hpp>
#endif

#define VK_CHECK(call) \
	do { \
		VkResult result_ = call; \
		assert(result_ == VK_SUCCESS); \
	} while (0)

#define CHECK(call) \
	do { \
		auto result_ = call; \
		assert(result_); \
	} while (0)

int main()
{

	CHECK(SDL_Init(SDL_INIT_VIDEO));
	VK_CHECK(volkInitialize());

	
	VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "vkR",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.pEngineName = "vkR Engine",
		.engineVersion = VK_MAKE_VERSION(0, 1, 0),
		.apiVersion = VK_API_VERSION_1_4,
	};

	uint32_t instanceExtensionsCount;
	char const* const* instanceExtensions{ SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount) };

	VkInstanceCreateInfo instanceCreateInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = instanceExtensionsCount,
		.ppEnabledExtensionNames = instanceExtensions,
	};

	VkInstance instance;
	VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
	volkLoadInstance(instance);

	VkSurfaceKHR surface;
	struct {
		int x, y;
	} windowSize;
	SDL_Window* window = SDL_CreateWindow("vkR", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	assert(window);
	CHECK(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));
	CHECK(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));

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
