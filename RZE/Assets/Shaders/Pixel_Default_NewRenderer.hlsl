struct PS_IN
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UVCoords : UV;
	float3 Tangent : TANGENT;
	float3 FragPos : POSITION;
	float3 CameraPos : POSITION1;
};

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

float4 PSMain(PS_IN input) : SV_TARGET
{
	float LightStrength_Temp = 10.0f;
	float3 LightPos_Temp = float3(0.0f, 5.0f, -20.0f);
	float3 LightColour_Temp = float3(1.0f, 1.0f, 1.0f);
	float3 Ambient_Temp = float3(0.1f, 0.1f, 0.1f);
	float3 ObjectColour_Temp = float3(0.5f, 0.5f, 0.5f);

	float3 lightDir = normalize(LightPos_Temp - input.FragPos);
	
	float3 shadingResult;
	{
		float diffuse = max(dot(lightDir, input.Normal), 0.0f);
		float3 diffuseResult = diffuse * LightColour_Temp;
		
		shadingResult = (Ambient_Temp + diffuseResult) * ObjectColour_Temp;
	}
	
	
    return float4(shadingResult, 1.0f);
}