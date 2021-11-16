#pragma once

#include "obt_pipeline.hpp"
#include "obt_device.hpp"
#include "obt_game_object.hpp"
#include "obt_camera.hpp"
#include "obt_frame_info.hpp"

#include <memory>
#include <vector>

namespace obt {

class SimpleRenderSystem {
	public:
		SimpleRenderSystem(ObtDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem &operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(FrameInfo& frameInfo, std::vector<ObtGameObject>& gameObjects);

	private:
		void createPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
		void createPipeline(VkRenderPass renderPass);

		ObtDevice& obtDevice;
		std::unique_ptr<ObtPipeline> obtPipeline;
		VkPipelineLayout pipelineLayout;
};

}
