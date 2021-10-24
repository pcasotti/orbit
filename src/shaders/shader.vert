#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

layout(set = 0, binding = 0) uniform CameraUbo {
	mat4 proj;
	mat4 view;
	mat4 projView;
} cameraUbo;

layout(set = 0, binding = 1) uniform SceneUbo {
	float ambient;
	vec3 lightDir;
} sceneUbo;

layout(location = 0) out vec3 fragColor;

const vec3 LIGHT_DIR = vec3(1.0, -3.0, -1.0);
const float AMBIENT = 0.02;

void main() {
	gl_Position = cameraUbo.projView * push.modelMatrix * vec4(position, 1.0);

	vec3 worldNormal = normalize(mat3(push.normalMatrix)*normal);

	float lightIntensity = sceneUbo.ambient + max(dot(worldNormal, sceneUbo.lightDir), 0);

	fragColor = color * lightIntensity;
}
