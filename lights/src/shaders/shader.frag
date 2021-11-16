#version 460

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 texCoord;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D tex;

layout(set = 0, binding = 1) uniform SceneUbo {
	float ambient;
	vec3 lightPos;
} scene;

struct LightData {
	vec3 position;
	vec4 color;
};

layout(std140, set = 2, binding = 0) readonly buffer LightSbo {
	LightData lights[];
} lightSbo;

void main() {
	vec3 diff = vec3(1.0);
	for (int i = 0; i < 3; ++i) {
		vec3 lightDir = normalize(lightSbo.lights[0].position-fragPos);
		vec3 d = vec3(max(dot(fragNormal, lightDir), 0.0));
		d *= lightSbo.lights[1].color.xyz;
		diff += d;
	}
	diff /= 3;

	outColor = vec4(scene.ambient+diff, 1.0) * texture(tex, texCoord);
}
