#include <vk_initializers.h>


VkCommandPoolCreateInfo vkinit::command_pool_create_info(uint32_t queue_family_index, VkCommandPoolCreateFlags flags) {


	VkCommandPoolCreateInfo command_pool_info = {};

	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.queueFamilyIndex = queue_family_index;
	command_pool_info.pNext = nullptr;

	return command_pool_info;
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1) {

	VkCommandBufferAllocateInfo cmd_allocate_info = {};
	cmd_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_allocate_info.pNext = nullptr;
	cmd_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_allocate_info.commandPool = pool;
	cmd_allocate_info.commandBufferCount = 1;

	return cmd_allocate_info;

}

VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags) {
	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags = flags;
	info.pNext = nullptr;

	return info;	
}

VkSemaphoreCreateInfo vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags) {

	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.flags = flags;
	info.pNext = nullptr;

	return info;
}

VkCommandBufferBeginInfo vkinit::commandBufferBeginInfo(VkCommandBufferUsageFlags flags) {

	VkCommandBufferBeginInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = flags;
	info.pNext = nullptr;
	info.pInheritanceInfo = nullptr;

	return info;
}

VkImageSubresourceRange vkinit::image_subresource_range(VkImageAspectFlags aspectMask)
{
	VkImageSubresourceRange subImage{};
	subImage.aspectMask = aspectMask;
	subImage.baseMipLevel = 0;
	subImage.levelCount = VK_REMAINING_MIP_LEVELS;
	subImage.baseArrayLayer = 0;
	subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

	return subImage;
}

VkSemaphoreSubmitInfo vkinit::semaphore_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore) {
	VkSemaphoreSubmitInfo info = {};

	info.pNext = nullptr;
	info.semaphore = semaphore;
	info.stageMask = stage_mask;
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	info.value = 1;
	info.deviceIndex = 0;

	return info;
}

VkCommandBufferSubmitInfo vkinit::command_buffer_submit_info(VkCommandBuffer cmd) {

	VkCommandBufferSubmitInfo info = {};
	info.commandBuffer = cmd;
	info.pNext = nullptr;
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	info.deviceMask = 0;

	return info;
}

VkSubmitInfo2 vkinit::submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* sig_semaphore_info, VkSemaphoreSubmitInfo* wait_semaphore_info) {

	VkSubmitInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	info.pNext = nullptr;
	info.waitSemaphoreInfoCount = wait_semaphore_info == nullptr? 0 : 1;
	info.pWaitSemaphoreInfos = wait_semaphore_info;

	info.signalSemaphoreInfoCount = sig_semaphore_info == nullptr ? 0 : 1;
	info.pSignalSemaphoreInfos = sig_semaphore_info;

	info.commandBufferInfoCount = 1;
	info.pCommandBufferInfos = cmd;

	return info;

}

VkImageCreateInfo vkinit::image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent) {

	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;

	info.arrayLayers = 1;

	info.extent = extent;
	info.format = format;

	info.imageType = VK_IMAGE_TYPE_2D;
	info.mipLevels = 1;

	info.samples = VK_SAMPLE_COUNT_1_BIT;

	info.tiling; VK_IMAGE_TILING_OPTIMAL;
	info.usage = usage_flags;

	return info;

}

VkImageViewCreateInfo vkinit::imageview_create_info(VkFormat format, VkImage img, VkImageAspectFlags aspet_flags) {


	VkImageViewCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = nullptr;

	info.format = format;
	info.image = img;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;

	info.subresourceRange.aspectMask = aspet_flags;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.levelCount = 1;

	return info;

}