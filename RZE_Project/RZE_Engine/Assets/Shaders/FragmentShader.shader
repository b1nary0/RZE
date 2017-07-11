#version 330

in vec4 FragmentColor;

out vec4 OutFragmentColor;

uniform vec4 UFragColor;

void main()
{
	OutFragmentColor = UFragColor;
}