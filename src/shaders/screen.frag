#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec4 fragLightSpace;
layout(location = 4) in vec3 fragNormal;

layout(set = 0, binding = 2) uniform sampler2D tex;
layout(set = 0, binding = 3) uniform sampler2D depth;

layout(set = 0, binding = 1) uniform SceneUbo {
	float ambient;
	float near;
	float far;
	vec3 lightDir;
} sceneUbo;

layout (location = 0) out vec4 outColor;

float normpdf(float x, float sigma) {
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

vec4 blur(sampler2D s, vec2 fragCoord) {
	//declare stuff
	const int mSize = 5;
	const int kSize = (mSize-1)/2;
	float kernel[mSize];
	vec3 final_colour = vec3(0.0);

	//create the 1-D kernel
	float sigma = 7.0;
	float Z = 0.0;
	for (int j = 0; j <= kSize; ++j) {
		kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), sigma);
	}

	//get the normalization factor (as the gaussian has been clamped)
	for (int j = 0; j < mSize; ++j) {
		Z += kernel[j];
	}

	//read out the texels
	vec2 texelSize = 1.0/textureSize(s, 0);
	for (int i = -kSize; i <= kSize; ++i) {
		for (int j = -kSize; j <= kSize; ++j) {
			final_colour += kernel[kSize+j]*kernel[kSize+i]*vec3(texture(s, fragCoord.xy+vec2(float(i),float(j))*texelSize).r);
		}
	}

	return vec4(final_colour/(Z*Z), 1.0);
}

float shadowLevel(vec4 fragPosLightSpace) {
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	float currentDepth = projCoords.z;

	// broken bias
	//float bias = max(0.01 * (1.0 - dot(fragNormal, sceneUbo.lightDir)), 0.001);
	float bias = 0.005;

	// used for pcf
	//vec2 texelSize = 1.0/textureSize(depth, 0);


	float depth = blur(depth, projCoords.xy).x;
	float shadow = currentDepth - bias > depth ? 1.0 : 0.0;

	return shadow;
}

void main() {
	float shadow = shadowLevel(fragLightSpace);

	outColor = vec4(vec3(sceneUbo.ambient+(1.0-shadow)), 1.0);
}
