#include "app.hpp"

#include "simple_render_system.hpp"
#include "offscreen_render_system.hpp"
#include "obt_camera.hpp"
#include "keyboard_controller.hpp"
#include "obt_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <chrono>
#include <numeric>

namespace obt {

struct CameraUbo {
	glm::mat4 proj{1.f};
	glm::mat4 view{1.f};
	glm::mat4 projView{1.f};
};

struct SceneUbo {
	float ambient = .04f;
	float near = 1.f;
	float far = 96.f;
	alignas(16) glm::vec3 lightDir{1.f, -3.f, -1.f};
};

struct ObjectSbo {
	glm::mat4 modelMatrix{1.f};
	glm::mat4 normalMatrix{1.f};
};

App::App() {
	globalPool = ObtDescriptorPool::Builder(obtDevice)
		.setMaxSets(ObtSwapChain::MAX_FRAMES_IN_FLIGHT * 10 * 4)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ObtSwapChain::MAX_FRAMES_IN_FLIGHT * 10)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, ObtSwapChain::MAX_FRAMES_IN_FLIGHT * 10)
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, ObtSwapChain::MAX_FRAMES_IN_FLIGHT * 10)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, ObtSwapChain::MAX_FRAMES_IN_FLIGHT * 10)
		.build();

	loadGameObjects();
	prepareOffscreen();
}

App::~App() {
	vkDestroyFramebuffer(obtDevice.device(), shadowBuffer, nullptr);
	vkDestroyRenderPass(obtDevice.device(), shadowPass, nullptr);
}

void App::prepareOffscreen() {
	VkFormat depthFormat = obtDevice.findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depth = std::make_unique<ObtImage>(obtDevice, OFFSCREEN_SIZE, OFFSCREEN_SIZE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &depthAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(obtDevice.device(), &renderPassInfo, nullptr, &shadowPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = shadowPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = depth->getImageView();
	framebufferInfo.width = OFFSCREEN_SIZE;
	framebufferInfo.height = OFFSCREEN_SIZE;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(obtDevice.device(), &framebufferInfo, nullptr, &shadowBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create framebuffer!");
	}
}

void App::run() {
	std::unique_ptr<ObtSampler> sampler = std::make_unique<ObtSampler>(obtDevice, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	std::unique_ptr<ObtImage> texture = std::make_unique<ObtImage>(obtDevice, "res/textures/texture.png", VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	auto minOffsetAlignment = std::lcm(
		obtDevice.properties.limits.minUniformBufferOffsetAlignment,
		obtDevice.properties.limits.nonCoherentAtomSize);

	std::vector<std::unique_ptr<ObtBuffer>> cameraUboBuffers(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	std::vector<std::unique_ptr<ObtBuffer>> shadowUboBuffers(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	std::vector<std::unique_ptr<ObtBuffer>> objectSboBuffers(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < cameraUboBuffers.size(); i++) {
		cameraUboBuffers[i] = std::make_unique<ObtBuffer>(obtDevice, sizeof(CameraUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		cameraUboBuffers[i]->map();

		shadowUboBuffers[i] = std::make_unique<ObtBuffer>(obtDevice, sizeof(CameraUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		shadowUboBuffers[i]->map();

		objectSboBuffers[i] = std::make_unique<ObtBuffer>(obtDevice, sizeof(ObjectSbo) * 10000, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		objectSboBuffers[i]->map();
	}
	std::unique_ptr<ObtBuffer> sceneUboBuffer = std::make_unique<ObtBuffer>(
		obtDevice, sizeof(SceneUbo), ObtSwapChain::MAX_FRAMES_IN_FLIGHT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, minOffsetAlignment);
	sceneUboBuffer->map();

	auto globalSetLayout = ObtDescriptorSetLayout::Builder(obtDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	auto shadowSetLayout = ObtDescriptorSetLayout::Builder(obtDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	auto objectSetLayout = ObtDescriptorSetLayout::Builder(obtDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.build();

	std::vector<VkDescriptorSet> globalDescriptorSets(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	std::vector<VkDescriptorSet> shadowDescriptorSets(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	std::vector<VkDescriptorSet> objectDescriptorSets(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < globalDescriptorSets.size(); i++) {
		auto cameraInfo = cameraUboBuffers[i]->descriptorInfo();
		auto shadowInfo = shadowUboBuffers[i]->descriptorInfo();
		auto sceneInfo = sceneUboBuffer->descriptorInfo(sizeof(SceneUbo), 0);
		auto imageInfo = texture->descriptorInfo(sampler->getSampler());
		auto offscreenInfo = depth->descriptorInfo(sampler->getSampler(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
		ObtDescriptorWriter(*globalSetLayout, *globalPool)
			.writeBuffer(0, &cameraInfo)
			.writeBuffer(1, &sceneInfo)
			.writeImage(2, &imageInfo)
			.writeImage(3, &offscreenInfo)
			.build(globalDescriptorSets[i]);

		ObtDescriptorWriter(*shadowSetLayout, *globalPool)
			.writeBuffer(0, &shadowInfo)
			.writeBuffer(1, &sceneInfo)
			.build(shadowDescriptorSets[i]);

		auto objectInfo = objectSboBuffers[i]->descriptorInfo(sizeof(ObjectSbo)*10000, 0);
		ObtDescriptorWriter(*objectSetLayout, *globalPool)
			.writeBuffer(0, &objectInfo)
			.build(objectDescriptorSets[i]);
	}

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout->getDescriptorSetLayout(), objectSetLayout->getDescriptorSetLayout()};
	SimpleRenderSystem simpleRenderSystem{obtDevice, obtRenderer.getSwapChainRenderPass(), descriptorSetLayouts};
	descriptorSetLayouts[0] = shadowSetLayout->getDescriptorSetLayout();
	OffscreenRenderSystem offscreenRenderSystem{obtDevice, shadowPass, descriptorSetLayouts};
	ObtCamera camera{};
	camera.setViewTarget(glm::vec3{-1.f, -2.f, -2.f}, glm::vec3{0.f, 0.f, 2.5f});
	ObtCamera light{};
	light.setViewTarget(glm::vec3{3.f, -9.f, -3.f}, glm::vec3{0.f, 0.f, 2.5f});

	auto viewerObject = ObtGameObject::createGameObject();
	KeyboardController cameraController{};

	float totalTime = 0.f;
	auto currentTime = std::chrono::high_resolution_clock::now();
	while(!obtWindow.shouldClose()) {
		glfwPollEvents();

		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime-currentTime).count();
		currentTime = newTime;
		totalTime += frameTime;

		cameraController.moveXZ(obtWindow.getGLFWwindow(), viewerObject, frameTime);
		camera.setViewQuat(viewerObject.transform.translation, viewerObject.transform.rotation);

		float aspect = obtRenderer.getAspectRation();
		camera.setPerspectiveProjection(glm::radians(60.f), aspect, .1f, 100.f);
		light.setOrthographicProjection(-10.f, 10.f, -10.f, 10.f, .1f, 30.f);
		glm::vec3 lightPos{sin(totalTime)*3.f, -9.f, cos(totalTime)*-3.f};
		light.setViewTarget(lightPos, glm::vec3{0.f});

		if (auto commandBuffer = obtRenderer.beginFrame()) {
			int frameIndex = obtRenderer.getFrameIndex();
			uint32_t dynamicOffsets = sceneUboBuffer->getAlignmentSize()*frameIndex;
			std::vector<VkDescriptorSet> descriptorSets{shadowDescriptorSets[frameIndex], objectDescriptorSets[frameIndex]};
			FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, descriptorSets, dynamicOffsets};

			CameraUbo camUbo{};
			camUbo.proj = light.getProjection();
			camUbo.view = light.getView();
			camUbo.projView = camUbo.proj*camUbo.view;
			shadowUboBuffers[frameIndex]->writeToBuffer(&camUbo);
			shadowUboBuffers[frameIndex]->flush();

			SceneUbo sceneUbo{};
			sceneUbo.lightDir = lightPos;
			sceneUboBuffer->writeToIndex(&sceneUbo, frameIndex);
			sceneUboBuffer->flushIndex(frameIndex);

			ObjectSbo* objectSbo = (ObjectSbo*)objectSboBuffers[frameIndex]->getMappedMemory();
			for (int i = 0; i < gameObjects.size(); i++) {
				auto& obj = gameObjects[i];
				objectSbo[i].modelMatrix = obj.transform.mat4();
				objectSbo[i].normalMatrix = obj.transform.normalMatrix();
			}

			std::vector<VkClearValue> clearValues{1};
			clearValues[0].depthStencil = {1.f, 0};
			obtRenderer.beginRenderPass(commandBuffer, shadowPass, shadowBuffer, {OFFSCREEN_SIZE, OFFSCREEN_SIZE}, clearValues);
			offscreenRenderSystem.renderGameObjects(frameInfo, gameObjects);
			obtRenderer.endRenderPass(commandBuffer, shadowPass);

			descriptorSets[0] = globalDescriptorSets[frameIndex];
			frameInfo.descriptorSets = descriptorSets;

			camUbo.projView = camera.getProjection()*camera.getView();
			cameraUboBuffers[frameIndex]->writeToBuffer(&camUbo);
			cameraUboBuffers[frameIndex]->flush();

			obtRenderer.beginSwapChainRenderPass(commandBuffer);
			simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
			obtRenderer.endSwapChainRenderPass(commandBuffer);
			obtRenderer.endFrame();
		}
	}

	vkDeviceWaitIdle(obtDevice.device());
}

void App::loadGameObjects() {
	std::shared_ptr<ObtModel> obtModel = ObtModel::createModelFromFile(obtDevice, "res/models/vulkan.obj");

	auto gameObject = ObtGameObject::createGameObject();
	gameObject.model = obtModel;
	gameObject.transform.translation = {0.f, -2.f, 2.5f};
	gameObject.transform.rotateLocalY(-glm::half_pi<float>());
	gameObjects.push_back(std::move(gameObject));
}

}
