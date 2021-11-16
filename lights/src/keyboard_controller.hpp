#pragma once

#include "obt_game_object.hpp"
#include "obt_window.hpp"

namespace obt {

class KeyboardController {
	public:
		struct KeyMappings {
			int moveLeft = GLFW_KEY_A;
			int moveRight = GLFW_KEY_D;
			int moveForward = GLFW_KEY_W;
			int moveBackward = GLFW_KEY_S;
			int moveUp = GLFW_KEY_E;
			int moveDown = GLFW_KEY_Q;
			int lookLeft = GLFW_KEY_LEFT;
			int lookRight = GLFW_KEY_RIGHT;
			int lookUp = GLFW_KEY_UP;
			int lookDown = GLFW_KEY_DOWN;
		};

		void moveXZ(GLFWwindow* window, ObtGameObject& gameObject, float dt);

		KeyMappings keys{};
		float moveSpeed = 3.f;
		float lookSpeed = 1.5f;
};

}
