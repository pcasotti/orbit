#version 460

layout(location = 0) in vec3 fragColor;

layout(set = 0, binding = 1) uniform SceneUbo {
	float ambient;
	vec3 lightDir;
} sceneUbo;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(fragColor, 1.0);
}
