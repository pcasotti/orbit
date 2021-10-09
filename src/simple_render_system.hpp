#pragma once

#include "obt_pipeline.hpp"
#include "obt_device.hpp"
#include "obt_game_object.hpp"
#include "obt_camera.hpp"

#include <memory>
#include <vector>

namespace obt {

class SimpleRenderSystem {
	public:
		SimpleRenderSystem(ObtDevice& device, VkRenderPass renderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem &operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<ObtGameObject>& gameObjects, const ObtCamera& camera);

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass);

		ObtDevice& obtDevice;
		std::unique_ptr<ObtPipeline> obtPipeline;
		VkPipelineLayout pipelineLayout;
};

}
