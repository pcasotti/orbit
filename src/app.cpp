#include "app.hpp"

#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>

namespace obt {

App::App() {
	loadGameObjects();
}

App::~App() {}

void App::run() {
	SimpleRenderSystem simpleRenderSystem{obtDevice, obtRenderer.getSwapChainRenderPass()};

	while(!obtWindow.shouldClose()) {
		glfwPollEvents();

		if (auto commandBuffer = obtRenderer.beginFrame()) {
			obtRenderer.beginSwapChainRenderPass(commandBuffer);
			simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
			obtRenderer.endSwapChainRenderPass(commandBuffer);
			obtRenderer.endFrame();
		}
	}

	vkDeviceWaitIdle(obtDevice.device());
}

void App::loadGameObjects() {
	std::vector<ObtModel::Vertex> vertices {
		{{.0f, -.5f}, {1.f, 0.f, 0.f}},
		{{.5f, .5f}, {0.f, 1.f, 0.f}},
		{{-.5f, .5f}, {0.f, 0.f, 1.f}},
	};
	auto obtModel = std::make_shared<ObtModel>(obtDevice, vertices);

	auto triangle = ObtGameObject::createGameObject();
	triangle.model = obtModel;
	triangle.color = {.1f, .8f, .1f};
	triangle.transform2d.translation.x = .2;
	triangle.transform2d.scale = {2.f, .5f};
	triangle.transform2d.rotation = glm::half_pi<float>();

	gameObjects.push_back(std::move(triangle));
}

}
