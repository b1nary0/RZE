#version 330

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 UModelMat;
uniform mat4 UProjectionMat;
uniform mat4 UViewMat;

void main()
{
	FragPos = vec3(UModelMat * vec4(VertexPosition, 1.0f));
	Normal = mat3(transpose(inverse(UModelMat))) * VertexNormal;
	
	gl_Position = UProjectionMat * UViewMat * vec4(FragPos, 1.0f);
}