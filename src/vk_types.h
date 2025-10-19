// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <glm/glm.hpp>  


struct AllocatedImage {

	VkImage image;
	VkImageView img_view;
	VmaAllocation allocation;
	VkExtent3D img_extent;
	VkFormat img_format;

};

struct AllocatedBuffer {

	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo info;
};

struct Vertex {

	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
};

struct GPUMeshBuffers {

	AllocatedBuffer index_buffer;
	AllocatedBuffer vertex_buffer;
	VkDeviceAddress vertex_buffer_addres;
};

struct GPUDrawPushConstants {

	glm::mat4 world_matrix;
	VkDeviceAddress vertex_buffer;
};