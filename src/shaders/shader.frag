#version 460

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D tex;

layout(set = 0, binding = 1) uniform SceneUbo {
	float ambient;
	vec3 lightPos;
	vec3 camPos;
} scene;

struct LightData {
	vec3 position;
	vec4 color;
};

layout(std140, set = 2, binding = 0) readonly buffer LightSbo {
	LightData lights[];
} lightSbo;

vec3 dirLightColor(LightData light, vec3 normal, vec3 lightDir, vec3 viewDir) {
	vec3 diff = light.color.xyz * vec3(max(dot(normal, lightDir), 0.0));

	float specularStrength = light.color.w;
	vec3 reflectionDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectionDir), 0.0), 32);
	vec3 specular = vec3(1.0) * spec * specularStrength;

	return light.color.w*scene.ambient + diff + specular;
}

vec3 pointLightColor(LightData light, vec3 normal, vec3 viewDir) {
	vec3 lightDir = light.position-fragPos;
	float attenuation = light.color.w / dot(lightDir, lightDir);
	lightDir = normalize(lightDir);
	vec3 diff = light.color.xyz * attenuation * vec3(max(dot(normal, lightDir), 0.0));

	float specularStrength = light.color.w;
	vec3 reflectionDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectionDir), 0.0), 32);
	vec3 specular = vec3(1.0) * attenuation * spec * specularStrength;

	return light.color.w*scene.ambient*attenuation + diff + specular;
}

void main() {
	vec3 lightDir = normalize(scene.lightPos-fragPos);
	vec3 viewDir = normalize(scene.camPos-fragPos);

	LightData dirLight = {scene.lightPos, vec4(1.0)};
	vec3 result = dirLightColor(dirLight, fragNormal, lightDir, viewDir);

	for (int i = 0; i < 3; ++i) {
		result += pointLightColor(lightSbo.lights[i], fragNormal, viewDir);
	}

	outColor = vec4(result, 1.0) * texture(tex, texCoord);
}
