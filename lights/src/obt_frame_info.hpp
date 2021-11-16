#pragma once

#include "obt_camera.hpp"

#include <vulkan/vulkan.h>
#include <vector>

namespace obt {

struct FrameInfo {
	int frameIndex;
	float frameTime;
	VkCommandBuffer commandBuffer;
	ObtCamera& camera;
	std::vector<VkDescriptorSet>& descriptorSets;
	uint32_t dynamicOffsets;
};

}
