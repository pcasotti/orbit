#include "app.hpp"

#include "simple_render_system.hpp"
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
	alignas(16) glm::vec3 lightDir{1.f, -3.f, -1.f};
};

App::App() {
	globalPool = ObtDescriptorPool::Builder(obtDevice)
		.setMaxSets(ObtSwapChain::MAX_FRAMES_IN_FLIGHT)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ObtSwapChain::MAX_FRAMES_IN_FLIGHT * 10)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, ObtSwapChain::MAX_FRAMES_IN_FLIGHT * 10)
		.build();

	loadGameObjects();
}

App::~App() {}

void App::run() {
	auto minOffsetAlignment = std::lcm(
		obtDevice.properties.limits.minUniformBufferOffsetAlignment,
		obtDevice.properties.limits.nonCoherentAtomSize);

	std::vector<std::unique_ptr<ObtBuffer>> cameraUboBuffers(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < cameraUboBuffers.size(); i++) {
		cameraUboBuffers[i] = std::make_unique<ObtBuffer>(obtDevice, sizeof(CameraUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		cameraUboBuffers[i]->map();
	}
	std::unique_ptr<ObtBuffer> sceneUboBuffer = std::make_unique<ObtBuffer>(
		obtDevice, sizeof(SceneUbo), ObtSwapChain::MAX_FRAMES_IN_FLIGHT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, minOffsetAlignment);
	sceneUboBuffer->map();

	auto globalSetLayout = ObtDescriptorSetLayout::Builder(obtDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	std::vector<VkDescriptorSet> globalDescriptorSets(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < globalDescriptorSets.size(); i++) {
		auto cameraInfo = cameraUboBuffers[i]->descriptorInfo();
		auto sceneInfo = sceneUboBuffer->descriptorInfo(sizeof(SceneUbo), 0);
		ObtDescriptorWriter(*globalSetLayout, *globalPool)
			.writeBuffer(0, &cameraInfo)
			.writeBuffer(1, &sceneInfo)
			.build(globalDescriptorSets[i]);
	}

	SimpleRenderSystem simpleRenderSystem{obtDevice, obtRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
	ObtCamera camera{};
	camera.setViewTarget(glm::vec3{-1.f, -2.f, -2.f}, glm::vec3{0.f, 0.f, 2.5f});

	auto viewerObject = ObtGameObject::createGameObject();
	KeyboardController cameraController{};

	auto currentTime = std::chrono::high_resolution_clock::now();
	while(!obtWindow.shouldClose()) {
		glfwPollEvents();

		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime-currentTime).count();
		currentTime = newTime;

		cameraController.moveXZ(obtWindow.getGLFWwindow(), viewerObject, frameTime);
		camera.setViewQuat(viewerObject.transform.translation, viewerObject.transform.rotation);

		float aspect = obtRenderer.getAspectRation();
		camera.setPerspectiveProjection(glm::radians(60.f), aspect, .1f, 100.f);

		if (auto commandBuffer = obtRenderer.beginFrame()) {
			int frameIndex = obtRenderer.getFrameIndex();
			uint32_t dynamicOffsets = sceneUboBuffer->getAlignmentSize()*frameIndex;
			FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], dynamicOffsets};

			CameraUbo camUbo{};
			camUbo.proj = camera.getProjection();
			camUbo.view = camera.getView();
			camUbo.projView = camUbo.proj*camUbo.view;
			cameraUboBuffers[frameIndex]->writeToBuffer(&camUbo);
			cameraUboBuffers[frameIndex]->flush();

			SceneUbo sceneUbo{};
			sceneUboBuffer->writeToIndex(&sceneUbo, frameIndex);
			sceneUboBuffer->flushIndex(frameIndex);

			obtRenderer.beginSwapChainRenderPass(commandBuffer);
			simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
			obtRenderer.endSwapChainRenderPass(commandBuffer);
			obtRenderer.endFrame();
		}
	}

	vkDeviceWaitIdle(obtDevice.device());
}

void App::loadGameObjects() {
	std::shared_ptr<ObtModel> obtModel = ObtModel::createModelFromFile(obtDevice, "res/models/smooth_vase.obj");

	auto gameObject = ObtGameObject::createGameObject();
	gameObject.model = obtModel;
	gameObject.transform.translation = {0.f, 0.f, 2.5f};
	gameObject.transform.scale = glm::vec3{2.f, 1.f, 2.f};
	gameObjects.push_back(std::move(gameObject));
}

}
