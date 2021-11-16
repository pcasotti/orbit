#include "obt_descriptors.hpp"

#include <cassert>
#include <stdexcept>

namespace obt {

ObtDescriptorSetLayout::Builder& ObtDescriptorSetLayout::Builder::addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count) {
	assert(bindings.count(binding) == 0 && "Binding already in use");
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = descriptorType;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags = stageFlags;
	bindings[binding] = layoutBinding;
	return *this;
}

std::unique_ptr<ObtDescriptorSetLayout> ObtDescriptorSetLayout::Builder::build() const {
	return std::make_unique<ObtDescriptorSetLayout>(obtDevice, bindings);
}

ObtDescriptorSetLayout::ObtDescriptorSetLayout(ObtDevice& obtDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings) : obtDevice{obtDevice}, bindings{bindings} {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
	for (auto kv : bindings) {
		setLayoutBindings.push_back(kv.second);
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(obtDevice.device(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

ObtDescriptorSetLayout::~ObtDescriptorSetLayout() {
	vkDestroyDescriptorSetLayout(obtDevice.device(), descriptorSetLayout, nullptr);
}

ObtDescriptorPool::Builder& ObtDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, uint32_t count) {
	poolSizes.push_back({descriptorType, count});
	return *this;
}

ObtDescriptorPool::Builder& ObtDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
	poolFlags = flags;
	return *this;
}
ObtDescriptorPool::Builder& ObtDescriptorPool::Builder::setMaxSets(uint32_t count) {
	maxSets = count;
	return *this;
}

std::unique_ptr<ObtDescriptorPool> ObtDescriptorPool::Builder::build() const {
	return std::make_unique<ObtDescriptorPool>(obtDevice, maxSets, poolFlags, poolSizes);
}

ObtDescriptorPool::ObtDescriptorPool(ObtDevice& obtDevice, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes) : obtDevice{obtDevice} {
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = maxSets;
	descriptorPoolInfo.flags = poolFlags;

	if (vkCreateDescriptorPool(obtDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

ObtDescriptorPool::~ObtDescriptorPool() {
	vkDestroyDescriptorPool(obtDevice.device(), descriptorPool, nullptr);
}

bool ObtDescriptorPool::allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	if (vkAllocateDescriptorSets(obtDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
		return false;
	}

	return true;
}

void ObtDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
	vkFreeDescriptorSets(obtDevice.device(), descriptorPool, static_cast<uint32_t>(descriptors.size()), descriptors.data());
}

void ObtDescriptorPool::resetPool() {
	vkResetDescriptorPool(obtDevice.device(), descriptorPool, 0);
}

ObtDescriptorWriter::ObtDescriptorWriter(ObtDescriptorSetLayout& setLayout, ObtDescriptorPool &pool) : setLayout{setLayout}, pool{pool} {}

ObtDescriptorWriter& ObtDescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
	assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

	auto& bindingDescription = setLayout.bindings[binding];

	assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pBufferInfo = bufferInfo;
	write.descriptorCount = 1;

	writes.push_back(write);
	return *this;
}

ObtDescriptorWriter& ObtDescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo) {
	assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

	auto &bindingDescription = setLayout.bindings[binding];

	assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pImageInfo = imageInfo;
	write.descriptorCount = 1;

	writes.push_back(write);
	return *this;
}

bool ObtDescriptorWriter::build(VkDescriptorSet& set) {
	bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
	if (!success) return false;
	overwrite(set);
	return true;
}

void ObtDescriptorWriter::overwrite(VkDescriptorSet& set) {
	for (auto& write : writes) {
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(pool.obtDevice.device(), writes.size(), writes.data(), 0, nullptr);
}

}
