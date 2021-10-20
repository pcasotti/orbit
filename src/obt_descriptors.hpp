#pragma once

#include "obt_device.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace obt {

class ObtDescriptorSetLayout {
	public:
		class Builder {
			public:
				Builder(ObtDevice &obtDevice) : obtDevice{obtDevice} {}

				Builder& addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
				std::unique_ptr<ObtDescriptorSetLayout> build() const;

			private:
				ObtDevice &obtDevice;
				std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
		};

		ObtDescriptorSetLayout(ObtDevice &obtDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
		~ObtDescriptorSetLayout();

		ObtDescriptorSetLayout(const ObtDescriptorSetLayout&) = delete;
		ObtDescriptorSetLayout &operator=(const ObtDescriptorSetLayout&) = delete;

		VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

	private:
		ObtDevice& obtDevice;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

	friend class ObtDescriptorWriter;
};

class ObtDescriptorPool {
	public:
		class Builder {
		public:
			Builder(ObtDevice &obtDevice) : obtDevice{obtDevice} {}

			Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
			Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
			Builder& setMaxSets(uint32_t count);
			std::unique_ptr<ObtDescriptorPool> build() const;

		private:
			ObtDevice &obtDevice;
			std::vector<VkDescriptorPoolSize> poolSizes{};
			uint32_t maxSets = 1000;
			VkDescriptorPoolCreateFlags poolFlags = 0;
		};

		ObtDescriptorPool(ObtDevice &obtDevice, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize> &poolSizes);
		~ObtDescriptorPool();

		ObtDescriptorPool(const ObtDescriptorPool&) = delete;
		ObtDescriptorPool &operator=(const ObtDescriptorPool&) = delete;

		bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

		void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

		void resetPool();

	private:
		ObtDevice& obtDevice;
		VkDescriptorPool descriptorPool;

	friend class ObtDescriptorWriter;
};

class ObtDescriptorWriter {
	public:
		ObtDescriptorWriter(ObtDescriptorSetLayout &setLayout, ObtDescriptorPool& pool);

		ObtDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		ObtDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

		bool build(VkDescriptorSet& set);
		void overwrite(VkDescriptorSet& set);

	private:
		ObtDescriptorSetLayout& setLayout;
		ObtDescriptorPool& pool;
		std::vector<VkWriteDescriptorSet> writes;
	};

}
