#version 440

#define MAX_LIGHT_SUPPORT 32

in vec3 Normal;
in vec3 FragPos;
in vec3 Tangent;
in vec2 UVCoord;

out vec4 OutFragmentColor;

layout (binding=0) uniform sampler2D DiffuseTexture;
layout (binding=1) uniform sampler2D SpecularTexture;
layout (binding=2) uniform sampler2D NormalMap;

uniform vec3 LightPositions[MAX_LIGHT_SUPPORT];
uniform vec3 LightColors[MAX_LIGHT_SUPPORT];
uniform float LightStrengths[MAX_LIGHT_SUPPORT];
uniform int UNumActiveLights;

uniform float UShininess;
uniform float UOpacity;

uniform vec3 ViewPos; // Cam pos

vec3 CalculateBumpNormal()
{
	vec3 normal = normalize(Normal);
	vec3 tangent = normalize(Tangent);
	tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
	
	vec3 biTangent = cross(Tangent, Normal);
	vec3 BumpMapNormal = texture(NormalMap, UVCoord).rgb;
	BumpMapNormal = 2.0 * BumpMapNormal - vec3(1.0, 1.0, 1.0);
	
	mat3 TBN = mat3(tangent, biTangent, Normal);
	vec3 newNormal = TBN * BumpMapNormal;
	newNormal = normalize(newNormal);
	
	return newNormal;
}

float CalculateBlinnPhong(vec3 viewDir, vec3 lightDir, vec3 normal)
{
	float specular = 0.0;
	
	vec3 halfDirection = normalize(lightDir + viewDir);
	float specAngle = max(0.0, dot(halfDirection, normal));
	specular = pow(specAngle, UShininess);
	
	return specular;
}

void main()
{
	vec4 surfaceColour = texture(DiffuseTexture, UVCoord);
	vec4 specularSample = texture(SpecularTexture, UVCoord);
	
	float ambientCoefficient = 0.175f;
	vec3 normal = CalculateBumpNormal();
	
	vec3 lightMix = vec3(0f, 0f, 0f);
	for (int lightIdx = 0; lightIdx < UNumActiveLights; ++lightIdx)
	{
		vec3 lightDir = normalize(LightPositions[lightIdx] - FragPos);
		vec3 viewDir = normalize(ViewPos - FragPos);
		
		float diff = max(0.0f, dot(normal, lightDir));
		float specular = CalculateBlinnPhong(viewDir, lightDir, normal);

		vec3 ambientResult = ambientCoefficient * surfaceColour.rgb;
		vec3 diffuseResult = ambientResult + (surfaceColour.rgb * LightColors[lightIdx] * LightStrengths[lightIdx] * diff);
		vec3 specularResult = specular * specularSample.xyz * LightStrengths[lightIdx];

		vec3 result = diffuseResult + specularResult;

		lightMix = lightMix + result;
	}

	OutFragmentColor = vec4(lightMix, UOpacity);
}
