#include "obt_renderer.hpp"

#include <stdexcept>
#include <array>

namespace obt {

ObtRenderer::ObtRenderer(ObtWindow& window, ObtDevice& device) : obtWindow{window}, obtDevice{device} {
	recreateSwapchain();
	createCommandBuffers();
}

ObtRenderer::~ObtRenderer() {
	freeCommandBuffers();
}

void ObtRenderer::recreateSwapchain() {
	auto extent = obtWindow.getExtent();
	while (extent.width == 0 || extent.height == 0) {
		extent = obtWindow.getExtent();
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(obtDevice.device());

	if (obtSwapChain == nullptr) {
		obtSwapChain = std::make_unique<ObtSwapChain>(obtDevice, extent);
	} else {
		std::shared_ptr<ObtSwapChain> oldSwapChain = std::move(obtSwapChain);
		obtSwapChain = std::make_unique<ObtSwapChain>(obtDevice, extent, oldSwapChain);

		if (!oldSwapChain->compareSwapFormats(*obtSwapChain.get())) {
			throw std::runtime_error("Swap chain image foramt has changed!");
		}
	}
}

void ObtRenderer::createCommandBuffers() {
	commandBuffers.resize(ObtSwapChain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = obtDevice.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(obtDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}
}

void ObtRenderer::freeCommandBuffers() {
	vkFreeCommandBuffers(obtDevice.device(), obtDevice.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	commandBuffers.clear();
}

VkCommandBuffer ObtRenderer::beginFrame() {
	assert(!isFrameStarted && "Cannot call beginFrame while already in progress");

	auto result = obtSwapChain->acquireNextImage(&currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	isFrameStarted = true;

	auto commandBuffer = getCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	return commandBuffer;
}

void ObtRenderer::endFrame() {
	assert(isFrameStarted && "Cannot call endFrame while frame is not in progress!");

	auto commandBuffer = getCurrentCommandBuffer();
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer!");
	}

	auto result = obtSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || obtWindow.wasWindowResized()) {
		obtWindow.resetWindowResizedFlag();
		recreateSwapchain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image!");
	}

	isFrameStarted = false;
	currentFrameIndex = (currentFrameIndex+1)%ObtSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void ObtRenderer::beginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D extent, std::vector<VkClearValue>& clearValues) {
	assert(isFrameStarted && "Cannot call beginSwapChainRenderPass while frame is not in progress!");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on a command buffer from a different frame!");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = frameBuffer;

	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = extent;

	/*
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {.01f, .01f, .01f, 1.f};
	clearValues[1].depthStencil = {1.f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	*/
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = extent.width;
	viewport.height = extent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	VkRect2D scissor{{0, 0}, extent};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void ObtRenderer::endRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass) {
	assert(isFrameStarted && "Cannot call endSwapChainRenderPass while frame is not in progress!");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on a command buffer from a different frame!");

	vkCmdEndRenderPass(commandBuffer);
}

void ObtRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
	std::vector<VkClearValue> clearValues{2};
	clearValues[0].color = {.01f, .01f, .01f, 1.f};
	clearValues[1].depthStencil = {1.f, 0};
	beginRenderPass(commandBuffer, obtSwapChain->getRenderPass(), obtSwapChain->getFrameBuffer(currentImageIndex), obtSwapChain->getSwapChainExtent(), clearValues);
}

void ObtRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
	endRenderPass(commandBuffer, obtSwapChain->getRenderPass());
}

}
