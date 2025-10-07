
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>

#include "VkBootstrap.h"


constexpr bool bUseValidationLayers = false;

void VulkanEngine::init()
{

	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
	
	_window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_windowExtent.width,
		_windowExtent.height,
		window_flags
	);

	init_vulkan();
	init_swapchain();
	init_commands();
	init_sync_structures();

	_isInitialized = true;
}
void VulkanEngine::cleanup()
{	
	if (_isInitialized) {
		
		vkDestroyInstance(instance, nullptr);
		SDL_DestroyWindow(_window);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(device, nullptr);

		vkb::destroy_debug_utils_messenger(instance, debug_messenger);
		
		SDL_DestroyWindow(_window);
	}
}

void VulkanEngine::draw()
{
	//nothing yet
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	//main loop
	while (!bQuit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//close the window when user alt-f4s or clicks the X button			
			if (e.type == SDL_QUIT) bQuit = true;
		}

		draw();
	}
}

void VulkanEngine::init_vulkan() {

	vkb::InstanceBuilder builder;

	builder.set_app_name("VULKAN GUIDE");
	builder.request_validation_layers(bUseValidationLayers);
	builder.use_default_debug_messenger();
	builder.require_api_version(1, 3, 0);
	auto inst_ret = builder.build();

	vkb::Instance vkb_inst = inst_ret.value();

	instance = vkb_inst.instance;
	debug_messenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(_window, instance, &surface);


	
	VkPhysicalDeviceVulkan13Features features{};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features.dynamicRendering = true;
	features.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features features12{ };
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	
	vkb::PhysicalDevice physical_device = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(surface)
		.select()
		.value();

	vkb::DeviceBuilder device_builder{ physical_device };
	vkb::Device vkb_device = device_builder.build().value();

	device = vkb_device.device;
	choosen_gpu = physical_device.physical_device;



}

void VulkanEngine::init_commands() {

}

void VulkanEngine::createSwapChain(uint32_t width, uint32_t height) {

	vkb::SwapchainBuilder swapchain_builder{ choosen_gpu, device, surface };
	swapchain_img_format = VK_FORMAT_B8G8R8A8_UNORM;


	VkSurfaceFormatKHR surface_format{};
	surface_format.format = swapchain_img_format;
	surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	swapchain_builder.set_desired_format(surface_format)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	vkb::Swapchain vkb_swapchain = swapchain_builder.build().value();

	swapchain_extend = vkb_swapchain.extent;
	swapchain = vkb_swapchain.swapchain;
	swapchain_imgs = vkb_swapchain.get_images().value();
	swapchain_img_views = vkb_swapchain.get_image_views().value();
}

void VulkanEngine::init_swapchain() {
	createSwapChain(_windowExtent.width, _windowExtent.height);
}

void VulkanEngine::init_sync_structures() {

}



void VulkanEngine::destroySwapChain() {
	vkDestroySwapchainKHR(device, swapchain, nullptr);

	for (int i = 0; i < swapchain_img_views.size(); i++) {

		vkDestroyImageView(device, swapchain_img_views[i], nullptr);
	}
}