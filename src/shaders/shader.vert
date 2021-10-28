#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(set = 0, binding = 0) uniform CameraUbo {
	mat4 proj;
	mat4 view;
	mat4 projView;
} cameraUbo;

layout(set = 0, binding = 1) uniform SceneUbo {
	float ambient;
	vec3 lightDir;
} sceneUbo;

struct ObjectData {
	mat4 modelMatrix;
	mat4 normalMatrix;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectSbo {
	ObjectData objects[];
} objectSbo;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 texCoord;

const vec3 LIGHT_DIR = vec3(1.0, -3.0, -1.0);
const float AMBIENT = 0.02;

void main() {
	mat4 modelMatrix = objectSbo.objects[gl_BaseInstance].modelMatrix;
	mat4 normalMatrix = objectSbo.objects[gl_BaseInstance].normalMatrix;

	gl_Position = cameraUbo.projView * modelMatrix * vec4(position, 1.0);

	vec3 worldNormal = normalize(mat3(normalMatrix)*normal);

	float lightIntensity = sceneUbo.ambient + max(dot(worldNormal, sceneUbo.lightDir), 0);

	fragColor = color * lightIntensity;
	texCoord = uv;
}
