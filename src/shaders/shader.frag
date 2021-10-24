#version 450

layout(location = 0) in vec3 fragColor;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

layout(set = 0, binding = 1) uniform SceneUbo {
	float ambient;
	vec3 lightDir;
} sceneUbo;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(fragColor, 1.0);
}
