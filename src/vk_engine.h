// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>

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

	VkSwapchainKHR swapchain;
	VkFormat swapchain_img_format;

	std::vector<VkImage> swapchain_imgs;
	std::vector<VkImageView> swapchain_img_views;
	VkExtent2D swapchain_extend;



	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void init();

	void createSwapChain(uint32_t width, uint32_t height);
	void destroySwapChain();

	void cleanup();

	void draw();

	void run();
};
