struct PS_IN
{
	float4 Position : SV_POSITION;
	float3 FragPos : POSITION;
	float3 Normal : NORMAL;
	float2 UVCoords : UV;
};

struct CAMERA_INPUT_DATA
{
	float3 Position;
};

struct MATERIAL_DATA
{
	float Shininess;
};

cbuffer CameraConstantBuffer : register(b0)
{
	CAMERA_INPUT_DATA CameraData;
};

cbuffer MaterialBuffer : register(b1)
{
	MATERIAL_DATA MaterialData;
}

Texture2D textures[3] : register(t0);
SamplerState samplerState : register(s0);

float CalculateBlinnPhong(float3 viewDir, float3 lightDir, float3 normal)
{
	float specular = 0.0f;
	
	 float3 halfDir = normalize(lightDir + viewDir);
	 float specAngle = saturate(max(0.0f, dot(halfDir, normal)));
	 specular = pow(specAngle, MaterialData.Shininess);
	
	return specular;
}

float4 PSMain(PS_IN input) : SV_TARGET
{
	float LightStrength_Temp = 10.0f;
	float3 LightPos_Temp = float3(0.0f, 5.0f, -20.0f);
	float3 LightColour_Temp = float3(1.0f, 1.0f, 1.0f);
	float3 Ambient_Temp = float3(0.1f, 0.1f, 0.1f);
	//float3 ObjectColour_Temp = float3(0.5f, 0.5f, 0.5f);

	float3 lightDir = LightPos_Temp - input.FragPos;
	float distance = length(lightDir);
	lightDir = normalize(lightDir);
	
	float3 viewDir = normalize(CameraData.Position - input.FragPos);
	
	float3 shadingResult;
	{
		float4 diffSample = textures[0].Sample(samplerState, input.UVCoords);
		float4 specularSample = textures[1].Sample(samplerState, input.UVCoords);
		
		float diffuse = max(dot(lightDir, input.Normal), 0.0f);
		float3 diffuseResult = diffuse * diffSample.rgb * LightColour_Temp;
		
		float specularValue = CalculateBlinnPhong(viewDir, lightDir, input.Normal);
		float3 specularResult = specularSample.rgb * specularValue;
		
		shadingResult = (Ambient_Temp + diffuseResult + specularResult);
	}
	
	
    return float4(shadingResult, 1.0f);
}