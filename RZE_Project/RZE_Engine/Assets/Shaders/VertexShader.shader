#version 330

layout(location = 0) in vec3 Position;
//layout(location = 1) in vec3 Normal;

uniform mat4 UModelViewProjection;

void main()
{
	gl_Position = UModelViewProjection * vec4(Position, 1.0f);
}