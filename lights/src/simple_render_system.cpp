#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>

namespace obt {

SimpleRenderSystem::SimpleRenderSystem(ObtDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) : obtDevice{device} {
	createPipelineLayout(descriptorSetLayouts);
	createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
	vkDestroyPipelineLayout(obtDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(obtDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout!");
	}
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

	PipelineConfigInfo pipelineConfig{};
	ObtPipeline::defaultPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	obtPipeline = std::make_unique<ObtPipeline>(obtDevice, "res/shaders/shader.vert.spv", "res/shaders/shader.frag.spv", pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo, std::vector<ObtGameObject>& gameObjects) {
	obtPipeline->bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.descriptorSets[0], 1, &frameInfo.dynamicOffsets);
	vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &frameInfo.descriptorSets[1], 0, nullptr);
	vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &frameInfo.descriptorSets[2], 0, nullptr);

	for (int i = 0; i < gameObjects.size(); i++) {
		auto& obj = gameObjects[i];

		obj.model->bind(frameInfo.commandBuffer);
		obj.model->draw(frameInfo.commandBuffer, i);
	}
}

}
