#pragma once

#include "obt_camera.hpp"

#include <vulkan/vulkan.h>

namespace obt {

struct FrameInfo {
	int frameIndex;
	float frameTime;
	VkCommandBuffer commandBuffer;
	ObtCamera& camera;
	VkDescriptorSet globalDescriptorSet;
};

}
