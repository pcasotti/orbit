#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 texCoord;

layout(set = 0, binding = 0) uniform CameraUbo {
	mat4 proj;
	mat4 view;
	mat4 projView;
} camera;

struct ObjectData {
	mat4 modelMatrix;
	mat4 normalMatrix;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectSbo {
	ObjectData objects[];
} objectSbo;

void main() {
	mat4 modelMatrix = objectSbo.objects[gl_BaseInstance].modelMatrix;
	mat4 normalMatrix = objectSbo.objects[gl_BaseInstance].normalMatrix;

	vec4 p = modelMatrix * vec4(position, 1.0);
	gl_Position = camera.projView * p;

	fragPos = p.xyz;
	fragNormal = normalize(mat3(normalMatrix)*normal);;
	texCoord = uv;
}
