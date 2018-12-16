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

void main()
{
	vec4 surfaceColour = texture(DiffuseTexture, UVCoord);
	vec4 specularSample = texture(SpecularTexture, UVCoord);
	
	float ambientCoefficient = 0.175f;
	vec3 normal = CalculateBumpNormal();
	
	vec3 lightMix = vec3(0f, 0f, 0f);
	for (int lightIdx = 0; lightIdx < UNumActiveLights; ++lightIdx)
	{
		vec3 surfaceToLight = normalize(LightPositions[lightIdx] - FragPos);
		vec3 reflectVec = reflect(-surfaceToLight, normal);
		vec3 surfaceToCamera = normalize(ViewPos - FragPos);
		
		float cosAngle = max(0.0, dot(surfaceToCamera, reflectVec));
		float specularCoefficient = pow(cosAngle, 0.25);
		float diff = max(0.0f, dot(normal, surfaceToLight));

		vec3 ambient = ambientCoefficient * surfaceColour.rgb;
		vec3 diffuse = ambient + (surfaceColour.rgb * LightColors[lightIdx] * LightStrengths[lightIdx] * diff);
		vec3 specular = specularCoefficient * specularSample.xyz * LightStrengths[lightIdx];

		vec3 result = diffuse + specular;

		lightMix = lightMix + result;
	}

	OutFragmentColor = vec4(lightMix, surfaceColour.a);
}
