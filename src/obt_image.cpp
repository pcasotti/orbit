#include "obt_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>

namespace obt {

ObtSampler::ObtSampler(ObtDevice& obtDevice, VkFilter filter, VkSamplerAddressMode addressMode, VkBool32 anisotropy) : obtDevice{obtDevice} {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;
	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;
	samplerInfo.anisotropyEnable = anisotropy;
	samplerInfo.maxAnisotropy = anisotropy ? obtDevice.properties.limits.maxSamplerAnisotropy : 1.f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.f;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 0.f;

	if (vkCreateSampler(obtDevice.device(), &samplerInfo, nullptr, &imageSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image sampler!");
	}
}

ObtSampler::~ObtSampler() {
	vkDestroySampler(obtDevice.device(), imageSampler, nullptr);
}

ObtImage::ObtImage(ObtDevice& obtDevice, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkFormat imageFormat, VkImageAspectFlags aspectMask) : obtDevice{obtDevice}, imageFormat{imageFormat} {
	createImage(width, height, usage);
	createImageView(aspectMask);
}

ObtImage::ObtImage(ObtDevice& obtDevice, const std::string& filePath, VkImageUsageFlags usage, VkFormat imageFormat) : obtDevice{obtDevice}, imageFormat{imageFormat} {
	createImageFromFile(filePath, usage);
	createImageView();
}

ObtImage::~ObtImage() {
	vkDestroyImageView(obtDevice.device(), imageView, nullptr);
	vkDestroyImage(obtDevice.device(), textureImage, nullptr);
	vkFreeMemory(obtDevice.device(), textureImageMemory, nullptr);
}

void ObtImage::createImage(uint32_t width, uint32_t height, VkImageUsageFlags usage) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = imageFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	obtDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
}

void ObtImage::createImageFromFile(const std::string& filePath, VkImageUsageFlags usage) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth*texHeight*4;

	if (!pixels) throw std::runtime_error("Failed to load texture file!");

	ObtBuffer stagingBuffer = ObtBuffer{obtDevice, imageSize, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
	stagingBuffer.map();
	stagingBuffer.writeToBuffer(pixels);
	stagingBuffer.unmap();

	stbi_image_free(pixels);

	createImage(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), usage);
	transitionImageLayout(imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	obtDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
	transitionImageLayout(imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void ObtImage::transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = obtDevice.beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = textureImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage, destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	obtDevice.endSingleTimeCommands(commandBuffer);
}

void ObtImage::createImageView(VkImageAspectFlags aspectMask) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imageFormat;
	viewInfo.subresourceRange.aspectMask = aspectMask;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(obtDevice.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture image view!");
	}
}

VkDescriptorImageInfo ObtImage::descriptorInfo(VkSampler sampler, VkImageLayout imageLayout) {
	return VkDescriptorImageInfo{sampler, imageView, imageLayout};
}

}
