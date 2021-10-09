#include "keyboard_controller.hpp"

#include <limits>

namespace obt {

void KeyboardController::moveXZ(GLFWwindow* window, ObtGameObject& gameObject, float dt) {
	glm::vec3 rotate{0.f};
	if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y -= 1.f;
	if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y += 1.f;
	if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
	if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

	if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
		gameObject.transform.eulerRotateGlobal(glm::vec3{0.f, rotate.y, 0.f}*lookSpeed*dt);
		gameObject.transform.eulerRotateLocal(glm::vec3{rotate.x, 0.f, 0.f}*lookSpeed*dt);
	}

	// https://stackoverflow.com/questions/21515341/rotate-a-unit-vector-by-a-given-quaternion
	const glm::vec3 forward = glm::vec3{
		2*(gameObject.transform.rotation.x*gameObject.transform.rotation.z + gameObject.transform.rotation.y*gameObject.transform.rotation.w),
		2*(gameObject.transform.rotation.y*gameObject.transform.rotation.z - gameObject.transform.rotation.x*gameObject.transform.rotation.w),
		1 - 2*(gameObject.transform.rotation.x*gameObject.transform.rotation.x + gameObject.transform.rotation.y*gameObject.transform.rotation.y),
	};
	const glm::vec3 right = glm::vec3{
		-1 + 2*(gameObject.transform.rotation.y*gameObject.transform.rotation.y + gameObject.transform.rotation.z*gameObject.transform.rotation.z),
		-2*(gameObject.transform.rotation.x*gameObject.transform.rotation.y + gameObject.transform.rotation.z*gameObject.transform.rotation.w),
		-2*(gameObject.transform.rotation.x*gameObject.transform.rotation.z - gameObject.transform.rotation.y*gameObject.transform.rotation.w),
	};
	const glm::vec3 up{0.f, -1.f, 0.f};

	glm::vec3 moveDir{0.f};
	if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forward;
	if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forward;
	if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir += right;
	if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir -= right;
	if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += up;
	if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= up;

	if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
		gameObject.transform.translation += moveSpeed*dt*glm::normalize(moveDir);
	}
}

}
