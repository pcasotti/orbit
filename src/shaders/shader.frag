#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 texCoord;

layout(set = 0, binding = 2) uniform sampler2D tex;

layout(set = 0, binding = 1) uniform SceneUbo {
	float ambient;
	vec3 lightDir;
} sceneUbo;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = texture(tex, texCoord) * vec4(fragColor, 1.0);
}
