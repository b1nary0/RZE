#version 440

layout(location = 0) in vec3 VertexPosition;

uniform mat4 UModelMat;
uniform mat4 ULightSpaceMatrix;

void main()
{
	vec3 vertexPosition = vec3(UModelMat * vec4(VertexPosition, 1.0f));
	gl_Position = ULightSpaceMatrix * UModelMat * vec4(vertexPosition, 1.0f);
}