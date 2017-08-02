#version 330

in vec3 Normal;
in vec3 FragPos;
in vec2 UVCoord;

out vec4 OutFragmentColor;

uniform vec3 ULightPosition;
uniform vec3 UViewPosition;
uniform vec3 ULightColor;
uniform float ULightStrength;

uniform vec4 UFragColor;

uniform sampler2D UTexture2D;

void main()
{
	//vec3 normal = normalize(Normal);
	//vec3 lightDir = normalize(ULightPosition - FragPos);
	//float diff = max(dot(normal, lightDir), 0.0);
	
	//float specularStrength = 0.75f;
    //vec3 viewDir = normalize(UViewPosition - FragPos);
    //vec3 reflectDir = reflect(-lightDir, normal);  
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    
	//vec4 texVec = texture(UTexture2D, UVCoord);
	//vec3 ambient = ULightStrength * ULightColor * vec3(texVec);
	//vec3 diffuse = diff * ULightColor * vec3(texVec);
	//vec3 specular = specularStrength * spec * ULightColor * vec3(texVec);  
	
	//vec3 result = (ambient + diffuse + specular) * UFragColor.xyz;
	OutFragmentColor = texture(UTexture2D, UVCoord);
}