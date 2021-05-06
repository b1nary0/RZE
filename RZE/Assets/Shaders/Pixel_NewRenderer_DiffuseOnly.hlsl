struct PS_IN
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UVCoords : UV;
	float3 Tangent : TANGENT;
	float3 FragPos : POSITION;
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

float4 PSMain(PS_IN input) : SV_TARGET
{
	float LightStrength_Temp = 10.0f;
	float3 LightPos_Temp = float3(0.0f, 5.0f, -20.0f);
	float3 LightColour_Temp = float3(1.0f, 1.0f, 1.0f);
	float3 Ambient_Temp = float3(0.1f, 0.1f, 0.1f);
	//float3 ObjectColour_Temp = float3(0.5f, 0.5f, 0.5f);

	float3 lightDir = LightPos_Temp - input.FragPos;
	lightDir = normalize(lightDir);
	
	float3 viewDir = normalize(CameraData.Position - input.FragPos);
	
	float3 shadingResult;
	{
		float4 diffSample = textures[0].Sample(samplerState, input.UVCoords);
		
		float3 normal = input.Normal;
		
		float diffuse = max(dot(lightDir, normal), 0.0f);
		float3 diffuseResult = diffuse * diffSample.rgb * LightColour_Temp;
				
		shadingResult = (Ambient_Temp + diffuseResult);
	}
	
	
    return float4(shadingResult, 1.0f);
}