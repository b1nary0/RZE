#version 330

in vec4 FragmentColor;
out vec4 OutFragmentColor;

uniform vec4 UFragColor;

uniform int UTest;

void main()
{
	OutFragmentColor = vec4(0.0f, 0.0f, 255.0f, 1.0f);
}