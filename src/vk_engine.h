// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include "vk_userfunctions.h"
#include <vk_types.h>
#include <vector>
#include <queue>
#include <functional>
#include "vk_descriptors.h"
#include <glm/glm.hpp>




constexpr unsigned int FRAME_OVERLAP = 2;

struct DeletionQueue
{
private:

	std::deque<std::function<void()>> deletors;

public:

	void pushFunction(std::function<void()>&& function) {

		deletors.push_back(function);
	}
	void flush() {

		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); // Llama a la función sacada de la cola 
		}
		deletors.clear();
	}


};

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
	const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
};


struct FrameData {
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	VkSemaphore swapchain_semaphore, render_semaphore;
	VkFence render_fence;

	DeletionQueue deletion_queue;
};


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

	FrameData frames[FRAME_OVERLAP];
	FrameData& getCurrentFrame() { return frames[_frameNumber % FRAME_OVERLAP];}

	VkQueue graphics_queue;
	uint32_t graphics_queue_family;

	DeletionQueue main_deletion_queue;

	VmaAllocator vma_allocator;

	AllocatedImage draw_image;
	VkExtent2D draw_extent;

	DescriptorAllocator global_descriptor_allocator;

	VkDescriptorSet draw_image_descriptors;
	VkDescriptorSetLayout draw_image_descriptor_layout;

	VkPipeline gradient_pipeline;
	VkPipelineLayout gradient_pipeline_layout;

	VkFence imm_fence;
	VkCommandBuffer imm_command_buffer;
	VkCommandPool imm_command_pool;

	bool stop_rendering =false;

	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{ 0 };


	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();
	void init_descriptors();
	void init_pipelines();
	void init_background_pipelines();


	void init();

	void createSwapChain(uint32_t width, uint32_t height);
	void destroySwapChain();

	void cleanup();

	void draw();
	void draw_background(VkCommandBuffer cmd);

	void run();

	// DEARIMGUI 

	void inmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	void init_imgui();

	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

};

