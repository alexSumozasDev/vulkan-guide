
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

	auto inst_ret = builder.set_app_name("VULKAN GUIDE");
	inst_ret.request_validation_layers(bUseValidationLayers);
	inst_ret.use_default_debug_messenger();
	inst_ret.require_api_version(1, 3, 0);
	inst_ret.build();

	vkb::Instance vkb_inst = inst_ret.value();

	instance = vkb_inst.instance;
	debug_messenger = vkb_inst.debug_messenger;


}

void VulkanEngine::init_commands() {

}

void VulkanEngine::init_swapchain() {

}

void VulkanEngine::init_sync_structures() {

}