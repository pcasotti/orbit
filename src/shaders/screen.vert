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
	float near;
	float far;
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
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec4 fragLightSpace;
layout(location = 4) out vec3 fragNormal;

const vec3 LIGHT_DIR = vec3(1.0, -3.0, -1.0);
const float AMBIENT = 0.02;

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
);

void main() {
	mat4 modelMatrix = objectSbo.objects[gl_BaseInstance].modelMatrix;
	mat4 normalMatrix = objectSbo.objects[gl_BaseInstance].normalMatrix;

	fragPos = vec3(modelMatrix*vec4(position, 1.0));
	gl_Position = cameraUbo.projView * vec4(fragPos, 1.0);
	fragLightSpace = (biasMat * cameraUbo.proj * cameraUbo.view) * vec4(fragPos, 1.0);
	fragPos = (cameraUbo.projView * vec4(fragPos, 1.0)).xyz;

	fragNormal = normalize(mat3(normalMatrix)*normal);

	float lightIntensity = sceneUbo.ambient + max(dot(fragNormal, sceneUbo.lightDir), 0);

	fragColor = color * lightIntensity;
	texCoord = uv;
}
