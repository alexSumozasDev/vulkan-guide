
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

#include <vk_pipelines.h>
#include <iostream>

#include <array>
#include <span>
#include <functional>
#include <thread>
#include <chrono>


#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"


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
	init_descriptors();
	init_pipelines();
	init_imgui();
	init_default_data();
	

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
	VK_CHECK(vkWaitForFences(device, 1, &getCurrentFrame().render_fence, true, 10000000));

	getCurrentFrame().deletion_queue.flush();

	VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().render_fence));

	uint32_t swapchain_img_idx;

	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 100000000, getCurrentFrame().swapchain_semaphore, nullptr, &swapchain_img_idx));


	VkCommandBuffer cmd = getCurrentFrame().command_buffer;

	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	draw_extent.width = draw_image.img_extent.width;
	draw_extent.height = draw_image.img_extent.height;

	VkCommandBufferBeginInfo cmd_begin_info = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

	vkutil::transition_image(cmd, draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	draw_background(cmd);

	vkutil::transition_image(cmd, draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

	draw_geometry(cmd);

	vkutil::transition_image(cmd, draw_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transition_image(cmd, swapchain_imgs[swapchain_img_idx], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


	vkutil::copy_image_to_image(cmd, draw_image.image, swapchain_imgs[swapchain_img_idx], draw_extent, swapchain_extend);

	vkutil::transition_image(cmd, swapchain_imgs[swapchain_img_idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	draw_imgui(cmd, swapchain_img_views[swapchain_img_idx]);

	vkutil::transition_image(cmd, swapchain_imgs[swapchain_img_idx], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

	//vkCmdClearColorImage(cmd, draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &background_color, 1, &background_range);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradient_pipeline);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradient_pipeline_layout, 0, 1, &draw_image_descriptors, 0, nullptr);

	ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];


	ComputePushConstants psc;
	psc.data1 = glm::vec4(1, 0, 0, 1);
	psc.data2 = glm::vec4(0, 0, 1, 1);

	vkCmdPushConstants(cmd, gradient_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);


	vkCmdDispatch(cmd, std::ceil(draw_extent.width / 16.f), std::ceil(draw_extent.height / 16.f), 1);
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

			if (e.type == SDL_WINDOWEVENT) {

				if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					stop_rendering = true;
				}
				if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
					stop_rendering = false;
				}
			}

			ImGui_ImplSDL2_ProcessEvent(&e);
		}

		if (stop_rendering) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();

		ImGui::NewFrame();

		if (ImGui::Begin("background")) {

			ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];

			ImGui::Text("Selected effect: ", selected.name);

			ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0, backgroundEffects.size() - 1);

			ImGui::InputFloat4("data1", (float*)&selected.data.data1);
			ImGui::InputFloat4("data2", (float*)&selected.data.data2);
			ImGui::InputFloat4("data3", (float*)&selected.data.data3);
			ImGui::InputFloat4("data4", (float*)&selected.data.data4);
		}
		ImGui::End();

		ImGui::Render();


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

	VK_CHECK(vkCreateCommandPool(device, &command_pool_info, nullptr, &imm_command_pool));

	VkCommandBufferAllocateInfo imm_allocate_info = vkinit::command_buffer_allocate_info(imm_command_pool, 1);

	VK_CHECK(vkAllocateCommandBuffers(device, &imm_allocate_info, &imm_command_buffer));

	main_deletion_queue.pushFunction([=]() {

		vkDestroyCommandPool(device, imm_command_pool, nullptr);
		});

}

void VulkanEngine::init_descriptors() {

	std::vector<DescriptorAllocator::PoolSizeRatio> sizes = { {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1} };

	global_descriptor_allocator.initPool(device, 10, sizes);

	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		draw_image_descriptor_layout = builder.build(device, VK_SHADER_STAGE_COMPUTE_BIT);
	}


	draw_image_descriptors = global_descriptor_allocator.allocate(device, draw_image_descriptor_layout);

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView = draw_image.img_view;

	VkWriteDescriptorSet drawImageWrite = {};
	drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImageWrite.pNext = nullptr;

	drawImageWrite.dstBinding = 0;
	drawImageWrite.dstSet = draw_image_descriptors;
	drawImageWrite.descriptorCount = 1;
	drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImageWrite.pImageInfo = &imgInfo;

	vkUpdateDescriptorSets(device, 1, &drawImageWrite, 0, nullptr);

	VkDevice dev = device;                         // copy the device handle
	VkDescriptorPool pool = global_descriptor_allocator.pool;
	VkDescriptorSetLayout layout = draw_image_descriptor_layout;

	// Optionally "detach" ownership from the allocator so it won't double-destroy later
	global_descriptor_allocator.pool = VK_NULL_HANDLE;

	main_deletion_queue.pushFunction([=]() {
		if (pool)   vkDestroyDescriptorPool(dev, pool, nullptr);
		if (layout) vkDestroyDescriptorSetLayout(dev, layout, nullptr);
		});


}

void VulkanEngine::init_pipelines() {
	init_background_pipelines();
	init_triangle_pipeline();
	init_mesh_pipeline();
}

void VulkanEngine::init_background_pipelines() {

	VkPipelineLayoutCreateInfo  compute_layout = {};
	compute_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compute_layout.pNext = nullptr;
	compute_layout.pSetLayouts = &draw_image_descriptor_layout;
	compute_layout.setLayoutCount = 1;
	
	VkPushConstantRange push_constants = {};
	push_constants.offset = 0;
	push_constants.size = sizeof(ComputePushConstants);
	push_constants.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	
	compute_layout.pPushConstantRanges = &push_constants;
	compute_layout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(device, &compute_layout, nullptr, &gradient_pipeline_layout));

	VkShaderModule compute_draw_shader;
	if (!vkutil::load_shader_module("../../shaders/gradient_color.comp.spv", device, &compute_draw_shader)) {

		std::string("ERROR");
	}

	VkShaderModule sky_shader;
	if (!vkutil::load_shader_module("../../shaders/sky.comp.spv", device, &sky_shader)) {

		std::string("ERROR");
	}


	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = compute_draw_shader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = gradient_pipeline_layout;
	computePipelineCreateInfo.stage = stageinfo;

	ComputeEffect gradient;
	gradient.layout = gradient_pipeline_layout;
	gradient.name = "gradient";
	gradient.data = {};

	gradient.data.data1 = glm::vec4(1, 0, 0, 1);

	gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient_pipeline));

	computePipelineCreateInfo.stage.module = sky_shader;

	ComputeEffect sky;
	sky.layout = gradient_pipeline_layout;
	sky.name = "sky";
	sky.data = {};

	sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

	VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));


	vkDestroyShaderModule(device, compute_draw_shader, nullptr);
	vkDestroyShaderModule(device, sky_shader, nullptr);

	backgroundEffects.push_back(gradient);
	backgroundEffects.push_back(sky);


	main_deletion_queue.pushFunction([=]() {
		vkDestroyPipelineLayout(device, gradient_pipeline_layout, nullptr);
		vkDestroyPipeline(device, sky.pipeline, nullptr);
		vkDestroyPipeline(device, gradient.pipeline, nullptr);
		});


}

void VulkanEngine::init_triangle_pipeline() {


	VkShaderModule triangle_frag_shader;
	if (!vkutil::load_shader_module("../../shaders/colored_triangle.frag.spv", device, &triangle_frag_shader)) {

		std::cout << "Errorrr while creating frag shader of triangle";
	}

	VkShaderModule triangle_vert_shader;

	if (!vkutil::load_shader_module("../../shaders/colored_triangle.vert.spv", device, &triangle_vert_shader)) {

		std::cout << "Error while creating vert shader triangle";
	}

	VkPipelineLayoutCreateInfo info = vkinit::pipeline_layout_create_info();

	VK_CHECK(vkCreatePipelineLayout(device, &info, nullptr, &triangle_pipeline_layout));

	PipelineBuilder pipeline_builder;
	pipeline_builder.pipeline_layout = triangle_pipeline_layout;
	pipeline_builder.setShaders(triangle_vert_shader, triangle_frag_shader);
	pipeline_builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	pipeline_builder.setPolygonMode(VK_POLYGON_MODE_FILL);
	pipeline_builder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipeline_builder.setMultisamplingNone();
	pipeline_builder.disableBlending();
	pipeline_builder.disableDepthTest();

	pipeline_builder.setColorAttachmentFormat(draw_image.img_format);
	pipeline_builder.setDepthFormat(VK_FORMAT_UNDEFINED);


	triangle_pipeline = pipeline_builder.build_pipeline(device);

	vkDestroyShaderModule(device, triangle_frag_shader, nullptr);
	vkDestroyShaderModule(device, triangle_vert_shader, nullptr);

	main_deletion_queue.pushFunction([&]() {

		vkDestroyPipelineLayout(device, triangle_pipeline_layout, nullptr);
		vkDestroyPipeline(device, triangle_pipeline, nullptr);
		});

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
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

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

	VK_CHECK(vkCreateFence(device, &f_info, nullptr, &imm_fence));

	main_deletion_queue.pushFunction([=]() {
		vkDestroyFence(device, imm_fence, nullptr);
		});

}



void VulkanEngine::destroySwapChain() {
	vkDestroySwapchainKHR(device, swapchain, nullptr);

	for (int i = 0; i < swapchain_img_views.size(); i++) {

		vkDestroyImageView(device, swapchain_img_views[i], nullptr);
	}
}

void VulkanEngine::inmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(device, 1, &imm_fence));
	VK_CHECK(vkResetCommandBuffer(imm_command_buffer, 0));

	VkCommandBuffer cmd = imm_command_buffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

	VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &submit, imm_fence));

	VK_CHECK(vkWaitForFences(device, 1, &imm_fence, true, 9999999999));
}

void VulkanEngine::init_imgui()
{

	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool));


	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = choosen_gpu;
	init_info.Device = device;
	init_info.Queue = graphics_queue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;

	//dynamic rendering parameters for imgui to use
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo = { };
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_img_format;


	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	//ImGui_ImplVulkan_CreateFontsTexture();


	main_deletion_queue.pushFunction([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(device, imguiPool, nullptr);
		});
}

void VulkanEngine::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = vkinit::rendering_info(swapchain_extend, &colorAttachment, nullptr, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd); 
}

void VulkanEngine::draw_geometry(VkCommandBuffer cmd) {

	VkRenderingAttachmentInfo color_attachment = vkinit::attachment_info(draw_image.img_view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	;

	VkRenderingInfo render_info = vkinit::rendering_info(draw_extent, &color_attachment, nullptr, nullptr);

	vkCmdBeginRendering(cmd, &render_info);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_pipeline);

	VkViewport  viewport = {};

	viewport.x = 0;
	viewport.y = 0;

	viewport.height = draw_extent.height;
	viewport.width = draw_extent.width;

	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.height = draw_extent.height;
	scissor.extent.width = draw_extent.width;

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	vkCmdDraw(cmd, 3, 1, 0, 0);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pipeline);

	GPUDrawPushConstants push_constants;
	push_constants.world_matrix = glm::mat4{ 1.f };
	push_constants.vertex_buffer = rectangle.vertex_buffer_addres;

	vkCmdPushConstants(cmd, mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(cmd, rectangle.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

	vkCmdEndRendering(cmd);

}

AllocatedBuffer VulkanEngine::createBuffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) {
	
	VkBufferCreateInfo info = {};

	info.pNext = nullptr;
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = alloc_size;
	info.usage = usage;
	
	VmaAllocationCreateInfo alloc_info = {};
	alloc_info.usage = memory_usage;
	alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	AllocatedBuffer new_buffer;

	VK_CHECK(vmaCreateBuffer(vma_allocator, &info, &alloc_info, &new_buffer.buffer, &new_buffer.allocation, &new_buffer.info));

	return new_buffer;
}

void VulkanEngine::destroy_buffer(const AllocatedBuffer& buffer) {

	vmaDestroyBuffer(vma_allocator, buffer.buffer, buffer.allocation);

}

GPUMeshBuffers VulkanEngine::uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices) {

	const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);

	const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers new_surface;

	new_surface.vertex_buffer = createBuffer(vertex_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	VkBufferDeviceAddressInfo device_address_info = {};
	device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	device_address_info.buffer = new_surface.vertex_buffer.buffer;

	new_surface.vertex_buffer_addres = vkGetBufferDeviceAddress(device, &device_address_info);

	new_surface.index_buffer = createBuffer(index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	AllocatedBuffer staging = createBuffer(vertex_buffer_size + index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data = staging.allocation->GetMappedData();

	memcpy(data, vertices.data(), vertex_buffer_size);

	memcpy((char*)data + vertex_buffer_size, indices.data(), index_buffer_size);

	inmediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertex_buffer_size;

		vkCmdCopyBuffer(cmd, staging.buffer, new_surface.vertex_buffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{ 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertex_buffer_size;
		indexCopy.size = index_buffer_size;

		vkCmdCopyBuffer(cmd, staging.buffer, new_surface.index_buffer.buffer, 1, &indexCopy);


		});

	destroy_buffer(staging);	

	return new_surface;
}

void VulkanEngine::init_mesh_pipeline() {


	VkShaderModule  mesh_frag_shader;

	if (!vkutil::load_shader_module("../../shaders/colored_triangle.frag.spv", device, &mesh_frag_shader)) {
		std::cout << "Error al hacer el frag shader";
	}

	VkShaderModule mesh_vert_shader;

	if (!vkutil::load_shader_module("../../shaders/colored_triangle_mesh.vert.spv", device, &mesh_vert_shader)) {
		std::cout << "Error al hacer el vert shader";
	}

	VkPushConstantRange buffer_range{};

	buffer_range.offset = 0;
	buffer_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	buffer_range.size = sizeof(GPUDrawPushConstants);

	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();

	pipeline_layout_info.pPushConstantRanges = &buffer_range;
	pipeline_layout_info.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &mesh_pipeline_layout));

	PipelineBuilder pipeline_builder;

	pipeline_builder.pipeline_layout = mesh_pipeline_layout;
	pipeline_builder.setShaders(mesh_vert_shader, mesh_frag_shader);
	pipeline_builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipeline_builder.setPolygonMode(VK_POLYGON_MODE_FILL);
	pipeline_builder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipeline_builder.setMultisamplingNone();
	pipeline_builder.disableBlending();
	pipeline_builder.disableDepthTest();

	pipeline_builder.setColorAttachmentFormat(draw_image.img_format);
	pipeline_builder.setDepthFormat(VK_FORMAT_UNDEFINED);

	mesh_pipeline = pipeline_builder.build_pipeline(device);

	vkDestroyShaderModule(device, mesh_frag_shader, nullptr);
	vkDestroyShaderModule(device, mesh_vert_shader, nullptr);

	main_deletion_queue.pushFunction([&]() {

		vkDestroyPipelineLayout(device, mesh_pipeline_layout, nullptr);
		vkDestroyPipeline(device, mesh_pipeline, nullptr);

		});

}

void VulkanEngine::init_default_data() {
	std::array<Vertex, 4>	rect_vertices;

	rect_vertices[0].position = { 0.5,-0.5, 0 };
	rect_vertices[1].position = { 0.5,0.5, 0 };
	rect_vertices[2].position = { -0.5,-0.5, 0 };
	rect_vertices[3].position = { -0.5,0.5, 0 };

	rect_vertices[0].color = { 0,0, 0,1 };
	rect_vertices[1].color = { 0.5,0.5,0.5 ,1 };
	rect_vertices[2].color = { 1,0, 0,1 };
	rect_vertices[3].color = { 0,1, 0,1 };

	std::array<uint32_t, 6> rect_indices;

	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	rectangle = uploadMesh(rect_indices, rect_vertices);


	main_deletion_queue.pushFunction([&]() {
		destroy_buffer(rectangle.index_buffer);
		destroy_buffer(rectangle.vertex_buffer);
		});

}