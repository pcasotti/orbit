#pragma once

#include "obt_device.hpp"

#include <string>
#include <vector>

namespace obt {

struct PipelineConfigInfo {
	PipelineConfigInfo(const PipelineConfigInfo&) = delete;
	PipelineConfigInfo &operator=(const PipelineConfigInfo&) = delete;

	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	std::vector<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	uint32_t subpass = 0;
};


class ObtPipeline {
	public:
		ObtPipeline(ObtDevice& device, const std::string& vertPath, const std::string& fragPath, const PipelineConfigInfo& configInfo);
		~ObtPipeline();

		ObtPipeline(const ObtPipeline&) = delete;
		ObtPipeline &operator=(const ObtPipeline&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

	private:
		static std::vector<char> readFile(const std::string& path);

		void createGraphicsPipeline(const std::string& vertPath, const std::string& fragPath, const PipelineConfigInfo& configInfo);
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		ObtDevice& obtDevice;
		VkPipeline graphicsPipeline;

		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
};

}
