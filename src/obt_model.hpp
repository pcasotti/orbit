#pragma once

#include "obt_device.hpp"
#include "obt_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace obt {

class ObtModel {
	public:
		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;
			glm::vec3 normal;
			glm::vec2 uv;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator==(const Vertex& other) const {
				return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
			}
		};

		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void loadModel(const std::string& filePath);
		};

		ObtModel(ObtDevice& obtDevice, const ObtModel::Builder& builder);
		~ObtModel();

		ObtModel(const ObtModel&) = delete;
		ObtModel &operator=(const ObtModel&) = delete;

		static std::unique_ptr<ObtModel> createModelFromFile(ObtDevice& device, const std::string& filePath);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer, uint32_t instance = 0);

	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		ObtDevice& obtDevice;

		std::unique_ptr<ObtBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<ObtBuffer> indexBuffer;
		uint32_t indexCount;
};

}
