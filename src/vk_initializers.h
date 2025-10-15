// vulkan_guide.h 
#pragma once

#include <vk_types.h>

namespace vkinit {


	VkCommandPoolCreateInfo command_pool_create_info(uint32_t queue_family_index, VkCommandPoolCreateFlags flags);
	VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count);

	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags);
	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags=0);

	VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);

	VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);

	VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore);
	VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);
	VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* sig_semaphore_info, VkSemaphoreSubmitInfo* wait_semaphore_info);

	VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags flags, VkExtent3D extent);

	VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage img, VkImageAspectFlags aspet_flags);

	VkRenderingAttachmentInfo attachment_info(VkImageView view, VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderingInfo rendering_info(
		VkExtent2D                               extent,
		const VkRenderingAttachmentInfo* color_attachments,
		const VkRenderingAttachmentInfo* depth_attachment,
		const VkRenderingAttachmentInfo* stencil_attachment);


}



