#include "obt_model.hpp"

#include "obt_utils.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <cassert>
#include <cstring>
#include <unordered_map>

namespace std {

template <>
struct hash<obt::ObtModel::Vertex> {
	size_t operator()(obt::ObtModel::Vertex const& vertex) const {
		size_t seed = 0;
		obt::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
		return seed;
	}
};

}

namespace obt {

ObtModel::ObtModel(ObtDevice& obtDevice, const ObtModel::Builder& builder) : obtDevice{obtDevice} {
	createVertexBuffers(builder.vertices);
	createIndexBuffers(builder.indices);
}

ObtModel::~ObtModel() {
	vkDestroyBuffer(obtDevice.device(), vertexBuffer, nullptr);
	vkFreeMemory(obtDevice.device(), vertexBufferMemory, nullptr);

	if (hasIndexBuffer) {
		vkDestroyBuffer(obtDevice.device(), indexBuffer, nullptr);
		vkFreeMemory(obtDevice.device(), indexBufferMemory, nullptr);
	}
}

std::unique_ptr<ObtModel> ObtModel::createModelFromFile(ObtDevice& device, const std::string& filePath) {
	Builder builder{};
	builder.loadModel(filePath);

	return std::make_unique<ObtModel>(device, builder);
}

void ObtModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
	vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3!");

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	obtDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(obtDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(obtDevice.device(), stagingBufferMemory);

	obtDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
	obtDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(obtDevice.device(), stagingBuffer, nullptr);
	vkFreeMemory(obtDevice.device(), stagingBufferMemory, nullptr);
}

void ObtModel::createIndexBuffers(const std::vector<uint32_t>& indices) {
	indexCount = static_cast<uint32_t>(indices.size());
	hasIndexBuffer = indexCount > 0;
	if (!hasIndexBuffer) return;

	VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	obtDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(obtDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(obtDevice.device(), stagingBufferMemory);

	obtDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
	obtDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(obtDevice.device(), stagingBuffer, nullptr);
	vkFreeMemory(obtDevice.device(), stagingBufferMemory, nullptr);
}

void ObtModel::draw(VkCommandBuffer commandBuffer) {
	if (hasIndexBuffer) {
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	} else {
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}
}

void ObtModel::bind(VkCommandBuffer commandBuffer) {
	VkBuffer buffers[] = {vertexBuffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	if (hasIndexBuffer) vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

std::vector<VkVertexInputBindingDescription> ObtModel::Vertex::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> ObtModel::Vertex::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
	attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
	attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

	return attributeDescriptions;
}

void ObtModel::Builder::loadModel(const std::string& filePath) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str())) {
		throw std::runtime_error(warn+err);
	}

	vertices.clear();
	indices.clear();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			if (index.vertex_index >= 0) {
				vertex.position = {
					attrib.vertices[3*index.vertex_index+0],
					attrib.vertices[3*index.vertex_index+1],
					attrib.vertices[3*index.vertex_index+2]
				};

				vertex.color = {
					attrib.colors[3*index.vertex_index+0],
					attrib.colors[3*index.vertex_index+1],
					attrib.colors[3*index.vertex_index+2]
				};
			}

			if (index.normal_index >= 0) {
				vertex.normal = {
					attrib.normals[3*index.normal_index+0],
					attrib.normals[3*index.normal_index+1],
					attrib.normals[3*index.normal_index+2]
				};
			}

			if (index.texcoord_index >= 0) {
				vertex.uv = {
					attrib.texcoords[2*index.texcoord_index+0],
					attrib.texcoords[2*index.texcoord_index+1],
				};
			}

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

}
