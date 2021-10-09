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

	void setEuler(glm::vec3 angles) {
		rotation *= glm::quat_cast(glm::orientate3(glm::vec3{angles.z, angles.x, angles.y}));
	}

	void rotateGlobalX(float angle) { rotation = glm::normalize(glm::angleAxis(angle, glm::vec3{1.f, 0.f, 0.f})*rotation); }
	void rotateGlobalY(float angle) { rotation = glm::normalize(glm::angleAxis(angle, glm::vec3{0.f, -1.f, 0.f})*rotation); }
	void rotateGlobalZ(float angle) { rotation = glm::normalize(glm::angleAxis(angle, glm::vec3{0.f, 0.f, 1.f})*rotation); }

	void rotateLocalX(float angle) { rotation = glm::normalize(rotation*glm::angleAxis(angle, glm::vec3{1.f, 0.f, 0.f})); }
	void rotateLocalY(float angle) { rotation = glm::normalize(rotation*glm::angleAxis(angle, glm::vec3{0.f, -1.f, 0.f})); }
	void rotateLocalZ(float angle) { rotation = glm::normalize(rotation*glm::angleAxis(angle, glm::vec3{0.f, 0.f, 1.f})); }

	void eulerRotateGlobal(glm::vec3 angles) {
		glm::quat quatX = glm::angleAxis(angles.x, glm::vec3{1.f, 0.f, 0.f});
		glm::quat quatY = glm::angleAxis(angles.y, glm::vec3{0.f, -1.f, 0.f});
		glm::quat quatZ = glm::angleAxis(angles.z, glm::vec3{0.f, 0.f, 1.f});
		rotation = glm::normalize(quatX*quatY*quatZ*rotation);
	}

	void eulerRotateLocal(glm::vec3 angles) {
		glm::quat quatX = glm::angleAxis(angles.x, glm::vec3{1.f, 0.f, 0.f});
		glm::quat quatY = glm::angleAxis(angles.y, glm::vec3{0.f, -1.f, 0.f});
		glm::quat quatZ = glm::angleAxis(angles.z, glm::vec3{0.f, 0.f, 1.f});
		rotation = glm::normalize(rotation*quatX*quatY*quatZ);
	}

	float yaw() { return glm::yaw(glm::normalize(rotation)); }
	float pitch() { return glm::pitch(glm::normalize(rotation)); }
	float roll() { return glm::roll(glm::normalize(rotation)); }

	glm::mat4 mat4() {
		return glm::translate(glm::mat4{1.f}, translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4{1.f}, scale);
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

		id_t getId() const { return id; }

		std::shared_ptr<ObtModel> model{};
		glm::vec3 color{};
		TransformComponent transform{};

	private:
		ObtGameObject(id_t objId) : id{objId} {}

		id_t id;
};

}
