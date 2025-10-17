#include <vk_pipelines.h>
#include <fstream>
#include <vk_initializers.h>


bool vkutil::load_shader_module(const char* filePath, VkDevice device,VkShaderModule* outShaderModule) {


	std::ifstream shader_file(filePath, std::ios::ate | std::ios::binary);

	if (!shader_file.is_open()) {
		return false;
	}

	size_t file_size = (size_t)shader_file.tellg();

	std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

	shader_file.seekg(0);

	shader_file.read((char*)buffer.data(), file_size);

	shader_file.close();

	VkShaderModuleCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.pNext = nullptr;
	info.codeSize = buffer.size() * sizeof(uint32_t);
	info.pCode = buffer.data();

	VkShaderModule shader_module;
	
	if (vkCreateShaderModule(device, &info, nullptr, &shader_module) != VK_SUCCESS) {

		return false;
	}

	*outShaderModule = shader_module;

	return true;
}


void PipelineBuilder::clear() {

	input_asssembly = {};
	input_asssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	color_blend_attachment = {};

	multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	pipeline_layout = {};

	depth_stencil = {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	render_info = {};
	render_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

	shader_stages.clear();
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device) {
	
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.pNext = nullptr;

	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;

	VkPipelineVertexInputStateCreateInfo vertex_in_info = {};
	vertex_in_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.pNext = &render_info;

	pipeline_info.stageCount = (uint32_t)shader_stages.size();
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.pVertexInputState = &vertex_in_info;
	pipeline_info.pInputAssemblyState = &input_asssembly;
	pipeline_info.pViewportState = &viewportState;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.layout = pipeline_layout;

	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo = {  };
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = &state[0];
	dynamicInfo.dynamicStateCount = 2;

	pipeline_info.pDynamicState = &dynamicInfo;

	VkPipeline new_pipeline;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &new_pipeline) != VK_SUCCESS) {

		std::cout << "Fallo al crear el pipeline";

		return VK_NULL_HANDLE;
	}
	else {
		return new_pipeline;
	}
}


void PipelineBuilder::setShaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader) {

	shader_stages.clear();

	shader_stages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader));

	shader_stages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader));


}

void PipelineBuilder::setInputTopology(VkPrimitiveTopology topology) {


	input_asssembly.topology = topology;
	input_asssembly.primitiveRestartEnable = VK_FALSE;

}

void PipelineBuilder::setPolygonMode(VkPolygonMode mode)
{
	rasterizer.polygonMode = mode;
	rasterizer.lineWidth = 1.f;
}

void PipelineBuilder::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
	rasterizer.cullMode = cullMode;
	rasterizer.frontFace = frontFace;
}

void PipelineBuilder::setMultisamplingNone()
{
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::disableBlending()
{
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
}

void PipelineBuilder::setColorAttachmentFormat(VkFormat format) {

	color_attachment_format = format;

	render_info.colorAttachmentCount = 1;
	render_info.pColorAttachmentFormats = &color_attachment_format;
}

void PipelineBuilder::setDepthFormat(VkFormat format) {

	render_info.depthAttachmentFormat = format;
}

void PipelineBuilder::disableDepthTest() {

	depth_stencil.depthTestEnable = VK_FALSE;
	depth_stencil.depthWriteEnable = VK_FALSE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;
	depth_stencil.front = {};
	depth_stencil.back = {};
	depth_stencil.minDepthBounds = 0.f;
	depth_stencil.maxDepthBounds = 1.f;
}