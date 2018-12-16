#version 440

#define MAX_LIGHT_SUPPORT 32

in vec3 Normal;
in vec3 FragPos;
in vec2 UVCoord;

out vec4 OutFragmentColor;

uniform sampler2D    DiffuseTexture;
uniform sampler2D	 SpecularTexture;

uniform vec3 LightPositions[MAX_LIGHT_SUPPORT];
uniform vec3 LightColors[MAX_LIGHT_SUPPORT];
uniform float LightStrengths[MAX_LIGHT_SUPPORT];
uniform int UNumActiveLights;

uniform vec3 ViewPos; // Cam pos

void main()
{
	vec4 surfaceColour = texture(SpecularTexture, UVCoord);
	vec4 specularSample = texture(DiffuseTexture, UVCoord);
	
	float ambientCoefficient = 0.1f;
	vec3 normal = normalize(Normal);
	
	vec3 lightMix = vec3(0f, 0f, 0f);
	for (int lightIdx = 0; lightIdx < UNumActiveLights; ++lightIdx)
	{
		vec3 surfaceToLight = normalize(LightPositions[lightIdx] - FragPos);
		vec3 reflectVec = reflect(-surfaceToLight, normal);
		vec3 surfaceToCamera = normalize(ViewPos - FragPos);
		
		float cosAngle = max(0.0, dot(surfaceToCamera, reflectVec));
		float specularCoefficient = pow(cosAngle, 1);
		float diff = max(0.0f, dot(normal, surfaceToLight));

		vec3 ambient = ambientCoefficient * surfaceColour.rgb;
		vec3 diffuse = ambient + (surfaceColour.rgb * LightColors[lightIdx] * LightStrengths[lightIdx] * diff);
		vec3 specular = specularCoefficient * specularSample.xyz * LightStrengths[lightIdx];

		vec3 result = diffuse + specular;

		lightMix = lightMix + result;
	}

	OutFragmentColor = vec4(lightMix, surfaceColour.a);
}
