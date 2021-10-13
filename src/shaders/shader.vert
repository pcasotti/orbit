#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(push_constant) uniform Push {
	mat4 transform;
	mat4 normalMatrix;
} push;

layout(location = 0) out vec3 fragColor;

const vec3 LIGHT_DIR = normalize(vec3(1.0, -3.0, -1.0));
const float AMBIENT = 0.02;

void main() {
	gl_Position = push.transform * vec4(position, 1.0);

	vec3 worldNormal = normalize(mat3(push.normalMatrix)*normal);

	float lightIntensity = AMBIENT +  max(dot(worldNormal, LIGHT_DIR), 0);

	fragColor = color * lightIntensity;
}
