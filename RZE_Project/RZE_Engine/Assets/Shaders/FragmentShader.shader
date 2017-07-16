#version 330

in vec4 FragmentColor;

out vec4 OutFragmentColor;

uniform vec3 ULightPosition;
uniform vec3 UViewPosition;
uniform vec3 ULightColor;
uniform float ULightStrength;

uniform vec4 UFragColor;

void main()
{
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * ULightColor;
	
	OutFragmentColor = UFragColor;
}