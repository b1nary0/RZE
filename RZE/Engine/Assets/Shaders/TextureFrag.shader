#version 440

#define MAX_LIGHT_SUPPORT 32

in vec3 Normal;
in vec3 FragPos;
in vec2 UVCoord;

out vec4 OutFragmentColor;

struct MaterialData
{
	sampler2D    DiffuseTexture;
	sampler2D	 SpecularTexture;
	float		 Shininess;
};

uniform int IsTextured; // #TODO(Josh::This should be replaced by CPU-side sending the MISSING TEXTURE texture over)

uniform MaterialData Material;

uniform vec3 LightPositions[MAX_LIGHT_SUPPORT];
uniform vec3 LightColors[MAX_LIGHT_SUPPORT];
uniform float LightStrengths[MAX_LIGHT_SUPPORT];
uniform int UNumActiveLights;

uniform vec3 ViewPos; // Cam pos

void main()
{
	if (IsTextured > 0)
	{
		vec3 normal = normalize(Normal);

		vec3 lightMix = vec3(0f, 0f, 0f);
		for (int lightIdx = 0; lightIdx < UNumActiveLights; ++lightIdx)
		{
			vec3 lightDir = normalize(LightPositions[lightIdx] - FragPos);
			vec3 viewDir = normalize(ViewPos - FragPos);
			vec3 halfwayDir = normalize(lightDir + viewDir);

			float diff = max(dot(normal, lightDir), 0.0f);
			float spec = pow(max(dot(normal, halfwayDir), 0.0f), 8.0f);

			vec3 diffuse = LightColors[lightIdx] * (diff * vec3(texture(Material.SpecularTexture, UVCoord)));
			vec3 specular = LightStrengths[lightIdx] * (spec * vec3(texture(Material.SpecularTexture, UVCoord)));

			vec3 result = diffuse + specular;

			lightMix = lightMix + result;
		}

		OutFragmentColor = vec4(lightMix, 1.0f);
	}
	else
	{
		OutFragmentColor = vec4(1.0, 0.5, 0.75, 1.0);
	}
}
