#pragma once

#include "obt_window.hpp"
#include "obt_device.hpp"
#include "obt_game_object.hpp"
#include "obt_renderer.hpp"
#include "obt_descriptors.hpp"
#include "obt_image.hpp"

#include <memory>
#include <vector>

namespace obt {

class App {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;
		static constexpr int OFFSCREEN_SIZE = 1024;

		App();
		~App();

		App(const App&) = delete;
		App &operator=(const App&) = delete;

		void run();

	private:
		void loadGameObjects();
		void prepareOffscreen();

		ObtWindow obtWindow{WIDTH, HEIGHT, "Orbit"};
		ObtDevice obtDevice{obtWindow};
		ObtRenderer obtRenderer{obtWindow, obtDevice};

		std::unique_ptr<ObtDescriptorPool> globalPool{};
		std::vector<ObtGameObject> gameObjects;

		std::unique_ptr<ObtImage> color;
		std::unique_ptr<ObtImage> depth;

		VkRenderPass shadowPass;
		VkFramebuffer shadowBuffer;
};

}
