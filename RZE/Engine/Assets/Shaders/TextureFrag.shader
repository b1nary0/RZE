#version 330

in vec3 Normal;
in vec3 FragPos;
in vec2 UVCoord;

out vec4 OutFragmentColor;

uniform vec3 ULightPosition;
uniform vec3 UViewPosition;
uniform vec3 ULightColor;
uniform float ULightStrength;

uniform vec4 UFragColor;

uniform sampler2D UTexture2D;

struct MaterialData
{
	sampler2D    Diffuse;
	sampler2D	 Specular;
	float		 Shininess;
};

uniform MaterialData Material;

float ambientStrength = 0.1f;

void main()
{
	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(ULightPosition - FragPos);
    vec3 viewDir = normalize(UViewPosition - FragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	
	float diff = max(dot(normal, lightDir), 0.0);
	vec4 texVec = texture(UTexture2D, UVCoord);
	
	float specularStrength = 0.75f;
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
    
	vec3 ambient = ULightStrength * ULightColor * vec3(texture(Material.Diffuse, UVCoord));
	vec3 diffuse = diff * ULightColor * vec3(texture(Material.Diffuse, UVCoord));
	vec3 specular = specularStrength * spec * ULightColor;//vec3(texture(Material.Specular, UVCoord));  
	
	vec3 result = (ambient + specular + diffuse);
	OutFragmentColor = vec4(result, 1.0f);
}