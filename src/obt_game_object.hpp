#pragma once

#include "obt_model.hpp"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <memory>

namespace obt {

struct TransformComponent {
	glm::vec3 translation{};
	glm::vec3 scale{1.f, 1.f, 1.f};
	glm::quat rotation{};

	void setEuler(glm::vec3 angles);

	void rotateGlobalX(float angle);
	void rotateGlobalY(float angle);
	void rotateGlobalZ(float angle);

	void rotateLocalX(float angle);
	void rotateLocalY(float angle);
	void rotateLocalZ(float angle);

	void eulerRotateGlobal(glm::vec3 angles);
	void eulerRotateLocal(glm::vec3 angles);

	float yaw();
	float pitch();
	float roll();

	glm::mat4 mat4();
	glm::mat3 normalMatrix();
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

		id_t getId() const { return id; }

		std::shared_ptr<ObtModel> model{};
		glm::vec3 color{};
		TransformComponent transform{};

	private:
		ObtGameObject(id_t objId) : id{objId} {}

		id_t id;
};

}
