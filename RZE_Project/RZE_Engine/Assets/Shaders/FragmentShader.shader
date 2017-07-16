#version 330

in vec3 Normal;
in vec3 FragPos;

out vec4 OutFragmentColor;

uniform vec3 ULightPosition;
uniform vec3 UViewPosition;
uniform vec3 ULightColor;
uniform float ULightStrength;

uniform vec4 UFragColor;

void main()
{
	vec3 ambient = ULightStrength * ULightColor;
	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(ULightPosition - FragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * ULightColor;
	
	vec3 result = (ambient + diffuse) * UFragColor.xyz;
	OutFragmentColor = vec4(result, 1.0f);
}