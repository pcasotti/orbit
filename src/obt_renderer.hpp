#pragma once

#include "obt_window.hpp"
#include "obt_device.hpp"
#include "obt_swap_chain.hpp"
#include "obt_model.hpp"

#include <memory>
#include <vector>
#include <cassert>

namespace obt {

class ObtRenderer {
	public:
		ObtRenderer(ObtWindow& window, ObtDevice& device);
		~ObtRenderer();

		ObtRenderer(const ObtRenderer&) = delete;
		ObtRenderer &operator=(const ObtRenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return obtSwapChain->getRenderPass(); }
		float getAspectRation() const { return obtSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame is not in progress!");
			return commandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame is not in progress!");
			return currentFrameIndex;
		}

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D extent, std::vector<VkClearValue>& clearValues);
		void endRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass);
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapchain();

		ObtWindow& obtWindow;
		ObtDevice& obtDevice;
		std::unique_ptr<ObtSwapChain> obtSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex = 0;
		bool isFrameStarted = false;
};

}
