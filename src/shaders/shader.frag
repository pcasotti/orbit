#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 texCoord;

layout(set = 0, binding = 1) uniform SceneUbo {
	float ambient;
	float near;
	float far;
	vec3 lightDir;
} scene;

//layout (location = 0) out vec4 outColor;

float linearDepth(float depth) {
	float z = depth * 2.0 - 1.0;
	return (2.0*scene.near*scene.far) / (scene.far+scene.near-z * (scene.far-scene.near));
}

void main() {
	// outColor = vec4(vec3(linearDepth(gl_FragCoord.z)/scene.far), 1.0);
}
