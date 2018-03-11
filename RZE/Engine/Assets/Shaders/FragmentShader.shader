#version 440

#define MAX_LIGHT_SUPPORT 32

in vec3 Normal;
in vec3 FragPos;

out vec4 OutFragmentColor;

uniform vec3 ULightPosition;
uniform vec3 UViewPosition;
uniform vec3 ULightColor;
uniform float ULightStrength;

uniform vec4 UFragColor;

uniform vec3 LightPositions[MAX_LIGHT_SUPPORT];
uniform vec3 LightColors[MAX_LIGHT_SUPPORT];
uniform float LightStrengths[MAX_LIGHT_SUPPORT];
uniform int UNumActiveLights;

uniform vec3 ViewPos; // Cam pos

void main()
{
	vec3 normal = normalize(Normal);
	vec3 viewDir = normalize(ViewPos - FragPos);

	vec3 lightMix;
	for (int lightIdx = 0; lightIdx < UNumActiveLights; ++lightIdx)
	{
		vec3 lightDir = normalize(LightPositions[lightIdx] - FragPos);
		vec3 halfwayDir = normalize(lightDir + viewDir);

		float diff = max(dot(normal, lightDir), 0.0f);
		//float spec = pow(max(dot(viewDir, halfwayDir), 0.0f), 16.0f);

		vec3 diffuse = LightColors[lightIdx] * diff;
		vec3 specular = LightStrengths[lightIdx] * 0.005f * LightColors[lightIdx];

		vec3 result = diffuse + specular;

		lightMix = lightMix + result;
	}

	OutFragmentColor = vec4(lightMix, 1.0f);
}
