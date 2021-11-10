#pragma once

#include "obt_device.hpp"
#include "obt_buffer.hpp"

#include <memory>

namespace obt {

// Implement builder

class ObtSampler {
	public:
		ObtSampler(ObtDevice& obtDevice, VkFilter filter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkBool32 anisotropy = VK_TRUE);
		~ObtSampler();

		ObtSampler(const ObtSampler&) = delete;
		ObtSampler &operator=(const ObtSampler&) = delete;

		VkSampler getSampler() const { return imageSampler; }

	private:
		ObtDevice& obtDevice;

		VkSampler imageSampler;
};

class ObtImage {
	public:
		ObtImage(ObtDevice& obtDevice, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
		ObtImage(ObtDevice& obtDevice, const std::string& filePath, VkImageUsageFlags usage, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB);
		~ObtImage();

		ObtImage(const ObtImage&) = delete;
		ObtImage &operator=(const ObtImage&) = delete;

		VkDescriptorImageInfo descriptorInfo(VkSampler sampler, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkImageView* getImageView() { return &imageView; }

	private:
		void createImage(uint32_t width, uint32_t height, VkImageUsageFlags usage);
		void createImageFromFile(const std::string& filePath, VkImageUsageFlags usage);
		void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void createImageView(VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

		ObtDevice& obtDevice;

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView imageView;
		VkFormat imageFormat;
};

}
