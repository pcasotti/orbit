#include "obt_window.hpp"

#include <stdexcept>

namespace obt {

ObtWindow::ObtWindow(int width, int height, std::string name) : width{width}, height{height}, name{name} {
	initWindow();
}

ObtWindow::~ObtWindow() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void ObtWindow::initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void ObtWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
	if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}
}

void ObtWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto obtWindow = reinterpret_cast<ObtWindow*>(glfwGetWindowUserPointer(window));
	obtWindow->framebufferResized = true;
	obtWindow->width = width;
	obtWindow->height = height;
}

}
