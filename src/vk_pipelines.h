#include "vk_engine.h"

namespace vkutil {

	bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);



}

class PipelineBuilder
{

public:
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

	VkPipelineInputAssemblyStateCreateInfo input_asssembly;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineColorBlendAttachmentState color_blend_attachment;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineLayout pipeline_layout;
	VkPipelineDepthStencilStateCreateInfo depth_stencil;
	VkPipelineRenderingCreateInfo render_info;
	VkFormat color_attachment_format;

	PipelineBuilder() { clear(); }

	void clear();

	VkPipeline build_pipeline(VkDevice device);

	void setShaders(VkShaderModule vertex_shader, VkShaderModule fragment_module);

	void setInputTopology(VkPrimitiveTopology topology);
	void setPolygonMode(VkPolygonMode mode);
	void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
	void setMultisamplingNone();
	void disableBlending();
	void setColorAttachmentFormat(VkFormat format);
	void setDepthFormat(VkFormat format);
	void disableDepthTest();
};