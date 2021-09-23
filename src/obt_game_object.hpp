#pragma once

#include "obt_model.hpp"

#include <memory>

namespace obt {

struct Transform2dComponent {
	glm::vec2 translation{};
	glm::vec2 scale{1.f, 1.f};
	float rotation;

	glm::mat2 mat2() {
		const float s = glm::sin(rotation);
		const float c = glm::cos(rotation);

		glm::mat2 rotMat{{c, s}, {-s, c}};

		glm::mat2 scaleMat{{scale.x, 0.f}, {0.f, scale.y}};
		return rotMat*scaleMat;
	}
};

class ObtGameObject {
	public:
		using id_t = unsigned int;

		static ObtGameObject createGameObject() {
			static id_t currentId = 0;
			return ObtGameObject(currentId++);
		}

		ObtGameObject(const ObtGameObject&) = delete;
		ObtGameObject &operator=(const ObtGameObject&) = delete;
		ObtGameObject(ObtGameObject&&) = default;
		ObtGameObject &operator=(ObtGameObject&&) = default;

		const id_t getId() const { return id; }

		std::shared_ptr<ObtModel> model{};
		glm::vec3 color{};
		Transform2dComponent transform2d;

	private:
		ObtGameObject(id_t objId) : id{objId} {}

		id_t id;
};

}
