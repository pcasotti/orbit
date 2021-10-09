#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>

namespace obt {

struct PushConstantData {
	glm::mat4 transform{1.f};
	alignas(16) glm::vec3 color;
};

SimpleRenderSystem::SimpleRenderSystem(ObtDevice& device, VkRenderPass renderPass) : obtDevice{device} {
	createPipelineLayout();
	createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
	vkDestroyPipelineLayout(obtDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout() {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstantData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

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

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<ObtGameObject>& gameObjects, const ObtCamera& camera) {
	obtPipeline->bind(commandBuffer);

	auto projectionView = camera.getProjection()*camera.getView();

	for (auto& obj : gameObjects) {
		PushConstantData push{};
		push.color = obj.color;
		push.transform = projectionView*obj.transform.mat4();

		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &push);

		obj.model->bind(commandBuffer);
		obj.model->draw(commandBuffer);
	}
}

}
