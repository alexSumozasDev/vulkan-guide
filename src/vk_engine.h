// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

class VulkanEngine {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice choosen_gpu;
	VkDevice device;
	VkSurfaceKHR surface;

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void init();

	void cleanup();

	void draw();

	void run();
};
