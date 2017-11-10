#version 330 core

in vec2 TexCoords;
out vec4 OutColor;

uniform sampler2D TextureSampler;
uniform vec3 UTextColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(TextureSampler, TexCoords).r);
    OutColor = vec4(UTextColor, 1.0) * sampled;
}  