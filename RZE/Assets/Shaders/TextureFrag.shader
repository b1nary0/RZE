#version 440

#define MAX_LIGHT_SUPPORT 32

in vec3 Normal;
in vec3 FragPos;
in vec3 Tangent;
in vec2 UVCoord;

in vec4 FragPosLightSpace;

out vec4 OutFragmentColor;

layout (binding=0) uniform sampler2D DiffuseTexture;
layout (binding=1) uniform sampler2D SpecularTexture;
layout (binding=2) uniform sampler2D NormalMap;
layout (binding=3) uniform sampler2D DepthMap;

uniform float UShininess;
uniform float UOpacity;
uniform int UIsNormalMapped;

uniform vec3 LightPositions[MAX_LIGHT_SUPPORT];
uniform vec3 LightColors[MAX_LIGHT_SUPPORT];
uniform float LightStrengths[MAX_LIGHT_SUPPORT];
uniform int UNumActiveLights;

uniform vec3 ViewPos; // Cam pos

vec3 CalculateBumpNormal()
{
	vec3 normal = normalize(Normal);
	vec3 tangent = normalize(Tangent);
	tangent = normalize(Tangent - dot(Tangent, normal) * normal);
	
	vec3 biTangent = cross(Tangent, normal);
	vec3 bumpMapNormal = texture(NormalMap, UVCoord).rgb;
	bumpMapNormal = 2.0 * bumpMapNormal - vec3(1.0, 1.0, 1.0);
	
	mat3 TBN = mat3(tangent, biTangent, normal);
	vec3 newNormal = TBN * bumpMapNormal;
	newNormal = normalize(newNormal);
	
	return newNormal;
}

float CalculateBlinnPhong(vec3 viewDir, vec3 lightDir, vec3 normal)
{
	float specular = 0.0;
	
	vec3 halfDirection = normalize(lightDir + viewDir);
	float specAngle = max(0.0, dot(halfDirection, reflect(-lightDir, normal)));
	specular = pow(specAngle, UShininess);
	
	return specular;
}

float CalculateShadowFromDepthMap(vec3 normal, vec3 lightDir)
{
	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	vec3 projectionCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
	projectionCoords = projectionCoords * 0.5 + 0.5;
	
	float closestDepth = texture(DepthMap, projectionCoords.xy).r;
	float currentDepth = projectionCoords.z;
	float shadowResult = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	
	if (projectionCoords.z > 1.0)
		return 0.0;

	return shadowResult;
}

void main()
{
	vec4 surfaceColour = texture(DiffuseTexture, UVCoord);
	vec4 specularSample = texture(SpecularTexture, UVCoord);
	
	float ambientCoefficient = 0.175f;

	vec3 normal;
	if (UIsNormalMapped == 1)
		normal = CalculateBumpNormal();
	else
		normal = normalize(Normal);
	
	vec3 lightMix = vec3(0f, 0f, 0f);
	for (int lightIdx = 0; lightIdx < UNumActiveLights; ++lightIdx)
	{
		vec3 lightDir = normalize(LightPositions[lightIdx] - FragPos);
		vec3 viewDir = normalize(ViewPos - FragPos);
		
		float diff = max(0.0f, dot(normal, lightDir));
		float specular = CalculateBlinnPhong(viewDir, lightDir, normal);

		vec3 ambientResult = ambientCoefficient * surfaceColour.rgb;
		vec3 diffuseResult = (surfaceColour.rgb * LightColors[lightIdx] * LightStrengths[lightIdx] * diff);
		vec3 specularResult = specular * specularSample.rgb * LightStrengths[lightIdx] * LightColors[lightIdx];

		float shadowCalc = CalculateShadowFromDepthMap(normal, lightDir);
		vec3 result = (ambientResult + (1.0 - shadowCalc)) * diffuseResult + specularResult;

		lightMix = lightMix + result;
	}

	OutFragmentColor = vec4(lightMix, UOpacity);
}
