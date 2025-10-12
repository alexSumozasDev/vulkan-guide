// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

struct AllocatedImage {

	VkImage image;
	VkImageView img_view;
	VmaAllocation allocation;
	VkExtent3D img_extent;
	VkFormat img_format;

};