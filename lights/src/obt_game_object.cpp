#include "obt_game_object.hpp"

namespace obt {

void TransformComponent::setEuler(glm::vec3 angles) {
	rotation *= glm::quat_cast(glm::orientate3(glm::vec3{angles.z, angles.x, angles.y}));
}

void TransformComponent::rotateGlobalX(float angle) { rotation = glm::normalize(glm::angleAxis(angle, glm::vec3{1.f, 0.f, 0.f})*rotation); }
void TransformComponent::rotateGlobalY(float angle) { rotation = glm::normalize(glm::angleAxis(angle, glm::vec3{0.f, -1.f, 0.f})*rotation); }
void TransformComponent::rotateGlobalZ(float angle) { rotation = glm::normalize(glm::angleAxis(angle, glm::vec3{0.f, 0.f, 1.f})*rotation); }

void TransformComponent::rotateLocalX(float angle) { rotation = glm::normalize(rotation*glm::angleAxis(angle, glm::vec3{1.f, 0.f, 0.f})); }
void TransformComponent::rotateLocalY(float angle) { rotation = glm::normalize(rotation*glm::angleAxis(angle, glm::vec3{0.f, -1.f, 0.f})); }
void TransformComponent::rotateLocalZ(float angle) { rotation = glm::normalize(rotation*glm::angleAxis(angle, glm::vec3{0.f, 0.f, 1.f})); }

void TransformComponent::eulerRotateGlobal(glm::vec3 angles) {
	glm::quat quatX = glm::angleAxis(angles.x, glm::vec3{1.f, 0.f, 0.f});
	glm::quat quatY = glm::angleAxis(angles.y, glm::vec3{0.f, -1.f, 0.f});
	glm::quat quatZ = glm::angleAxis(angles.z, glm::vec3{0.f, 0.f, 1.f});
	rotation = glm::normalize(quatX*quatY*quatZ*rotation);
}

void TransformComponent::eulerRotateLocal(glm::vec3 angles) {
	glm::quat quatX = glm::angleAxis(angles.x, glm::vec3{1.f, 0.f, 0.f});
	glm::quat quatY = glm::angleAxis(angles.y, glm::vec3{0.f, -1.f, 0.f});
	glm::quat quatZ = glm::angleAxis(angles.z, glm::vec3{0.f, 0.f, 1.f});
	rotation = glm::normalize(rotation*quatX*quatY*quatZ);
}

float TransformComponent::yaw() { return glm::yaw(glm::normalize(rotation)); }
float TransformComponent::pitch() { return glm::pitch(glm::normalize(rotation)); }
float TransformComponent::roll() { return glm::roll(glm::normalize(rotation)); }

glm::mat4 TransformComponent::mat4() {
	return glm::translate(glm::mat4{1.f}, translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4{1.f}, scale);
}

glm::mat3 TransformComponent::normalMatrix() {
	return glm::mat4_cast(rotation) * glm::scale(glm::mat4{1.f}, 1.f/scale);
}

}
