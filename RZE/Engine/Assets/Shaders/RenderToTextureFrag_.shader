#version 440

in vec3 Normal;
in vec3 FragPos;
in vec2 UVCoord;

out vec3 OutFragmentColor;

uniform sampler2D UTexture2D;

void main()
{
	OutFragmentColor = texture(UTexture2D, UVCoord).xyz;
}