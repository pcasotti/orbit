#pragma once

#include "obt_device.hpp"
#include "obt_buffer.hpp"

#include <memory>

namespace obt {

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
		ObtImage(ObtDevice& obtDevice, const std::string& filePath, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB);
		~ObtImage();

		ObtImage(const ObtImage&) = delete;
		ObtImage &operator=(const ObtImage&) = delete;

		VkDescriptorImageInfo descriptorInfo(VkSampler sampler);

	private:
		void createImageFromFile(const std::string& filePath);
		void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void createImageView();

		ObtDevice& obtDevice;

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView imageView;
		VkFormat imageFormat;
};

}
