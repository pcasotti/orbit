#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projView;
	vec3 lightDir;
} ubo;

layout(location = 0) out vec3 fragColor;

const float AMBIENT = 0.02;

void main() {
	gl_Position = ubo.projView * push.modelMatrix * vec4(position, 1.0);

	vec3 worldNormal = normalize(mat3(push.normalMatrix)*normal);

	float lightIntensity = AMBIENT +  max(dot(worldNormal, ubo.lightDir), 0);

	fragColor = color * lightIntensity;
}
