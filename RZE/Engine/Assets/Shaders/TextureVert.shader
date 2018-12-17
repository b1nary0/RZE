#version 440

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec3 VertexTangent;
layout(location = 3) in vec2 VertexUV;

out vec3 FragPos;
out vec3 Normal;
out vec3 Tangent;
out vec2 UVCoord;

out vec4 FragPosLightSpace;

uniform mat4 UModelMat;
uniform mat4 UProjectionMat;
uniform mat4 UViewMat;
uniform mat4 ULightSpaceMat;

void main()
{
	FragPos = vec3(UModelMat * vec4(VertexPosition, 1.0f));
	Normal = mat3(transpose(inverse(UModelMat))) * VertexNormal;
	
	Tangent = VertexTangent;
	UVCoord = VertexUV;
	
	FragPosLightSpace = ULightSpaceMat * vec4(FragPos, 1.0);
	
	gl_Position = UProjectionMat * UViewMat * vec4(FragPos, 1.0f);
}