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
	matrix ClipSpace;
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

struct LightData
{
	float3 position;
	float4 colour;
	float strength;
};

cbuffer LightBuffer : register(b2)
{
	LightData lightData;
}

Texture2D textures[3] : register(t0);
SamplerState samplerState : register(s0);

float4 PSMain(PS_IN input) : SV_TARGET
{
	float minDiffuseFactor = 0.2f;
	
	float3 Ambient_Temp = float3(0.1f, 0.1f, 0.1f);

	float3 lightDir = lightData.position - input.FragPos;
	lightDir = normalize(lightDir);
	
	float3 viewDir = normalize(CameraData.Position - input.FragPos);
	
	float3 shadingResult;
	{
		float4 diffSample = textures[0].Sample(samplerState, input.UVCoords);
		
		float3 normal = input.Normal;
		
		float diffuse = max(dot(lightDir, normal), minDiffuseFactor);
		float3 diffuseResult = (diffuse * diffSample.rgb) * lightData.colour;
				
		shadingResult = (Ambient_Temp + diffuseResult);
	}
	
	
    return float4(shadingResult, 1.0f);
}