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

namespace obt {

struct GlobalUbo {
	glm::mat4 projView{1.f};
	glm::vec3 lightDir = glm::normalize(glm::vec3{1.f, -3.f, -1.f});
};

App::App() {
	loadGameObjects();
}

App::~App() {}

void App::run() {
	std::vector<std::unique_ptr<ObtBuffer>> globalUboBuffers(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < globalUboBuffers.size(); i++) {
		globalUboBuffers[i] = std::make_unique<ObtBuffer>(obtDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		globalUboBuffers[i]->map();
	}

	SimpleRenderSystem simpleRenderSystem{obtDevice, obtRenderer.getSwapChainRenderPass()};
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
			FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera};

			GlobalUbo ubo{};
			ubo.projView = camera.getProjection()*camera.getView();
			globalUboBuffers[frameIndex]->writeToBuffer(&ubo);
			globalUboBuffers[frameIndex]->flush();

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
	gameObject.transform.scale = glm::vec3{2.f, .5f, 2.f};
	gameObjects.push_back(std::move(gameObject));
}

}
