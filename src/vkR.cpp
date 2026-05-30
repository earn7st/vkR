#include <iostream>
#include <assert.h>

#include <GLFW/glfw3.h>

#if 0
#include <vulkan/vulkan.h>
#else
#include <vulkan/vulkan.hpp>
#endif
int main()
{

	int rc = glfwInit();
	assert(rc == GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(1024, 768, "vkR", nullptr, nullptr);  
	assert(window);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);



    std::cout << "Hello World!\n";
}
