#version 460

in vec3 Normal;
in vec3 FragPos;
in vec2 UVCoord;

out vec4 OutFragmentColor;

struct MaterialData
{
	sampler2D    DiffuseTextures[12];
	sampler2D	 SpecularTextures[12];
	float		 Shininess;
};

uniform vec3  ULightPosition;
uniform vec3  UViewPosition;
uniform vec3  ULightColor;
uniform vec4  UFragColor;
uniform float ULightStrength;

uniform int DiffuseTextureCount;
uniform int SpecularTextureCount;

uniform MaterialData Material;

void main()
{
	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(ULightPosition - FragPos);
    vec3 viewDir = normalize(UViewPosition - FragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	
	float diff = max(dot(normal, lightDir), 0.0f);
	
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), 8.0f);
    
	vec3 diffuse = ULightColor * (diff *  vec3(texture(Material.SpecularTextures[1], UVCoord)));
	vec3 specular = ULightStrength * (spec * vec3(texture(Material.DiffuseTextures[1], UVCoord)));  
	
	vec3 result = (specular + diffuse);
	OutFragmentColor = vec4(result, 1.0f);
}