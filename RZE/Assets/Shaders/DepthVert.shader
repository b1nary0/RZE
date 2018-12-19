#version 440

layout(location = 0) in vec3 VertexPosition;

uniform mat4 UModelMat;
uniform mat4 ULightSpaceMatrix;

void main()
{
	gl_Position = ULightSpaceMatrix * UModelMat * vec4(VertexPosition, 1.0f);
}