#include "app.hpp"

#include "simple_render_system.hpp"
#include "obt_camera.hpp"
#include "keyboard_controller.hpp"
#include "obt_buffer.hpp"
#include "obt_image.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <chrono>
#include <numeric>

namespace obt {

struct CameraData {
	glm::mat4 proj{1.f};
	glm::mat4 view{1.f};
	glm::mat4 projView{1.f};
};

struct SceneData {
	float ambient = .04f;
	alignas(16) glm::vec3 lightPos{1.f, -3.f, -1.f};
	alignas(16) glm::vec3 camPos{};
};

struct ObjectData {
	glm::mat4 modelMatrix{1.f};
	glm::mat4 normalMatrix{1.f};
};

struct LightData {
	glm::vec3 position{-1.f};
	alignas(16) glm::vec4 color{1.f};
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
}

App::~App() {}

void App::run() {
	std::unique_ptr<ObtSampler> sampler = std::make_unique<ObtSampler>(obtDevice);
	std::unique_ptr<ObtImage> texture = std::make_unique<ObtImage>(obtDevice, "res/textures/teapot.jpg");

	auto minOffsetAlignment = std::lcm(
		obtDevice.properties.limits.minUniformBufferOffsetAlignment,
		obtDevice.properties.limits.nonCoherentAtomSize);

	std::vector<std::unique_ptr<ObtBuffer>> cameraUboBuffers(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	std::vector<std::unique_ptr<ObtBuffer>> objectSboBuffers(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	std::vector<std::unique_ptr<ObtBuffer>> lightSboBuffers(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < cameraUboBuffers.size(); ++i) {
		cameraUboBuffers[i] = std::make_unique<ObtBuffer>(obtDevice, sizeof(CameraData), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		cameraUboBuffers[i]->map();

		objectSboBuffers[i] = std::make_unique<ObtBuffer>(obtDevice, sizeof(ObjectData) * 10000, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		objectSboBuffers[i]->map();

		lightSboBuffers[i] = std::make_unique<ObtBuffer>(obtDevice, sizeof(LightData) * 100, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		lightSboBuffers[i]->map(); }
	std::unique_ptr<ObtBuffer> sceneUboBuffer = std::make_unique<ObtBuffer>(
		obtDevice, sizeof(SceneData), ObtSwapChain::MAX_FRAMES_IN_FLIGHT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, minOffsetAlignment);
	sceneUboBuffer->map();

	auto globalSetLayout = ObtDescriptorSetLayout::Builder(obtDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	auto objectSetLayout = ObtDescriptorSetLayout::Builder(obtDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.build();

	auto lightSetLayout = ObtDescriptorSetLayout::Builder(obtDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	std::vector<VkDescriptorSet> globalDescriptorSets(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	std::vector<VkDescriptorSet> objectDescriptorSets(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	std::vector<VkDescriptorSet> lightDescriptorSets(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < globalDescriptorSets.size(); ++i) {
		auto cameraInfo = cameraUboBuffers[i]->descriptorInfo();
		auto sceneInfo = sceneUboBuffer->descriptorInfo(sizeof(SceneData), 0);
		auto imageInfo = texture->descriptorInfo(sampler->getSampler());
		ObtDescriptorWriter(*globalSetLayout, *globalPool)
			.writeBuffer(0, &cameraInfo)
			.writeBuffer(1, &sceneInfo)
			.writeImage(2, &imageInfo)
			.build(globalDescriptorSets[i]);

		auto objectInfo = objectSboBuffers[i]->descriptorInfo(sizeof(ObjectData)*10000, 0);
		ObtDescriptorWriter(*objectSetLayout, *globalPool)
			.writeBuffer(0, &objectInfo)
			.build(objectDescriptorSets[i]);

		auto lightInfo = lightSboBuffers[i]->descriptorInfo(sizeof(LightData)*100, 0);
		ObtDescriptorWriter(*lightSetLayout, *globalPool)
			.writeBuffer(0, &lightInfo)
			.build(lightDescriptorSets[i]);
	}

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		globalSetLayout->getDescriptorSetLayout(),
		objectSetLayout->getDescriptorSetLayout(),
		lightSetLayout->getDescriptorSetLayout()};
	SimpleRenderSystem simpleRenderSystem{obtDevice, obtRenderer.getSwapChainRenderPass(), descriptorSetLayouts};
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
			std::vector<VkDescriptorSet> descriptorSets{
				globalDescriptorSets[frameIndex],
				objectDescriptorSets[frameIndex],
				lightDescriptorSets[frameIndex]};
			FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, descriptorSets, dynamicOffsets};

			CameraData camData{};
			camData.proj = camera.getProjection();
			camData.view = camera.getView();
			camData.projView = camData.proj*camData.view;
			cameraUboBuffers[frameIndex]->writeToBuffer(&camData);
			cameraUboBuffers[frameIndex]->flush();

			SceneData sceneData{};
			sceneData.camPos = viewerObject.transform.translation;
			sceneUboBuffer->writeToIndex(&sceneData, frameIndex);
			sceneUboBuffer->flushIndex(frameIndex);

			ObjectData* objectData = (ObjectData*)objectSboBuffers[frameIndex]->getMappedMemory();
			for (int i = 0; i < gameObjects.size(); ++i) {
				auto& obj = gameObjects[i];
				objectData[i].modelMatrix = obj.transform.mat4();
				objectData[i].normalMatrix = obj.transform.normalMatrix();
			}

			LightData* lightData = (LightData*)lightSboBuffers[frameIndex]->getMappedMemory();
			for (int i = 0; i < pointLights.size(); ++i) {
				auto& light = pointLights[i];
				lightData[i].position = light.transform.translation;
				lightData[i].color = glm::vec4{light.color.x, light.color.y, light.color.z, 1.f};
			}

			obtRenderer.beginSwapChainRenderPass(commandBuffer);
			simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
			obtRenderer.endSwapChainRenderPass(commandBuffer);
			obtRenderer.endFrame();
		}
	}

	vkDeviceWaitIdle(obtDevice.device());
}

void App::loadGameObjects() {
	std::shared_ptr<ObtModel> floorModel = ObtModel::createModelFromFile(obtDevice, "res/models/floor.obj");
	std::shared_ptr<ObtModel> teapotModel = ObtModel::createModelFromFile(obtDevice, "res/models/teapot.obj");

	auto floor = ObtGameObject::createGameObject();
	floor.model = floorModel;
	gameObjects.push_back(std::move(floor));

	auto teapot = ObtGameObject::createGameObject();
	teapot.model = teapotModel;
	gameObjects.push_back(std::move(teapot));

	for (int i = 0; i < 2; ++i) {
		auto light = ObtGameObject::createGameObject();
		light.transform.translation = glm::vec3{-1+2*i, -1.f, 0.f};
		light.color = glm::vec3{i, 1-i, 0.f};
		pointLights.push_back(std::move(light));
	}
}

}
