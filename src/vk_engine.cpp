
#include "vk_engine.h"
#include "vk_images.h"

#include <SDL.h>
#include <SDL_vulkan.h>


#include <vk_types.h>
#include <vk_initializers.h>

#include "VkBootstrap.h"
#include <cmath>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"



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
		
		vkDeviceWaitIdle(device);

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			vkDestroyCommandPool(device, frames[i].command_pool, nullptr);

			vkDestroyFence(device, frames[i].render_fence, nullptr);
			vkDestroySemaphore(device, frames[i].render_semaphore, nullptr);
			vkDestroySemaphore(device, frames[i].swapchain_semaphore, nullptr);

			frames[i].deletion_queue.flush();
		}

		main_deletion_queue.flush();

		SDL_DestroyWindow(_window);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(device, nullptr);

		vkb::destroy_debug_utils_messenger(instance, debug_messenger);
		vkDestroyInstance(instance, nullptr);
		SDL_DestroyWindow(_window);


	}
}

void VulkanEngine::draw()
{
	VK_CHECK(vkWaitForFences(device, 1, &getCurrentFrame().render_fence, true, 1000000));

	getCurrentFrame().deletion_queue.flush();

	VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().render_fence));

	uint32_t swapchain_img_idx;

	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 10000000, getCurrentFrame().swapchain_semaphore, nullptr, &swapchain_img_idx));

	VkCommandBuffer cmd = getCurrentFrame().command_buffer;

	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	draw_extent.width = draw_image.img_extent.width;
	draw_extent.height = draw_image.img_extent.height;

	VkCommandBufferBeginInfo cmd_begin_info = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

	vkutil::transition_image(cmd, draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	draw_background(cmd);

	vkutil::transition_image(cmd, draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	vkutil::transition_image(cmd, swapchain_imgs[swapchain_img_idx], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	vkutil::copy_image_to_image(cmd, draw_image.image, swapchain_imgs[swapchain_img_idx], draw_extent, swapchain_extend);

	vkutil::transition_image(cmd, swapchain_imgs[swapchain_img_idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo  cmd_submit_info = vkinit::command_buffer_submit_info(cmd);
	VkSemaphoreSubmitInfo wait_submit_Info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchain_semaphore);
	VkSemaphoreSubmitInfo signal_submit_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().render_semaphore);

	VkSubmitInfo2 submit_info = vkinit::submit_info(&cmd_submit_info, &signal_submit_info, &wait_submit_Info);

	VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &submit_info, getCurrentFrame().render_fence));

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pImageIndices = &swapchain_img_idx;
	present_info.pNext = nullptr;
	present_info.pSwapchains = &swapchain;
	present_info.pWaitSemaphores = &getCurrentFrame().render_semaphore;
	present_info.swapchainCount = 1;
	present_info.waitSemaphoreCount = 1;

	VK_CHECK(vkQueuePresentKHR(graphics_queue, &present_info));

	_frameNumber++;

}

void VulkanEngine::draw_background(VkCommandBuffer cmd) {

	VkClearColorValue background_color;
	float flash = std::abs(std::sin(_frameNumber / 120.f));
	background_color = { 0.f, 0.f, flash, 0.f };

	VkImageSubresourceRange background_range = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

	vkCmdClearColorImage(cmd, draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &background_color, 1, &background_range);
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


	graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
	graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocator_info = {};
	allocator_info.physicalDevice = choosen_gpu;
	allocator_info.device = device;
	allocator_info.instance = instance;
	allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	vmaCreateAllocator(&allocator_info, &vma_allocator);

	main_deletion_queue.pushFunction([&]() {vmaDestroyAllocator(vma_allocator); });

}

void VulkanEngine::init_commands() {

	VkCommandPoolCreateInfo command_pool_info = vkinit::command_pool_create_info(graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool(device, &command_pool_info, nullptr, &frames[i].command_pool));

		VkCommandBufferAllocateInfo cmd_allocate_info = vkinit::command_buffer_allocate_info(frames[i].command_pool, 1);

		VK_CHECK(vkAllocateCommandBuffers(device, &cmd_allocate_info, &frames[i].command_buffer));
	}


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

	VkExtent3D draw_image_extent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	draw_image.img_format = VK_FORMAT_R16G16B16A16_SFLOAT;
	draw_image.img_extent = draw_image_extent;

	VkImageUsageFlags draw_image_usages{};
	draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	draw_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
	draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = vkinit::image_create_info(draw_image.img_format, draw_image_usages, draw_image_extent);

	VmaAllocationCreateInfo rimg_allocinfo = {};

	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vmaCreateImage(vma_allocator, &rimg_info, &rimg_allocinfo, &draw_image.image, &draw_image.allocation, nullptr);

	VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(draw_image.img_format, draw_image.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(device, &rview_info, nullptr, &draw_image.img_view));

	main_deletion_queue.pushFunction([=]() {
		vkDestroyImageView(device, draw_image.img_view, nullptr);
		vmaDestroyImage(vma_allocator, draw_image.image, draw_image.allocation);
		});

}

void VulkanEngine::init_sync_structures() {

	VkFenceCreateInfo f_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo s_info = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(device, &f_info, nullptr, &frames[i].render_fence));

		VK_CHECK(vkCreateSemaphore(device, &s_info, nullptr, &frames[i].render_semaphore));
		VK_CHECK(vkCreateSemaphore(device, &s_info, nullptr, &frames[i].swapchain_semaphore));
	}
}



void VulkanEngine::destroySwapChain() {
	vkDestroySwapchainKHR(device, swapchain, nullptr);

	for (int i = 0; i < swapchain_img_views.size(); i++) {

		vkDestroyImageView(device, swapchain_img_views[i], nullptr);
	}
}