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