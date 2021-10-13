#include "app.hpp"

#include "simple_render_system.hpp"
#include "obt_camera.hpp"
#include "keyboard_controller.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <chrono>

namespace obt {

App::App() {
	loadGameObjects();
}

App::~App() {}

void App::run() {
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
			obtRenderer.beginSwapChainRenderPass(commandBuffer);
			simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
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
	gameObject.transform.scale = glm::vec3{2.f};
	gameObjects.push_back(std::move(gameObject));
}

}
