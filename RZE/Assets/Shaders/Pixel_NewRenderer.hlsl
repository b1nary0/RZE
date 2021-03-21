struct PS_IN
{
	float4 Position : SV_POSITION;
	float3 FragPos : POSITION;
	float3 Normal : NORMAL;
};

float4 PSMain(PS_IN input) : SV_TARGET
{
	float3 LightPos_Temp = float3(0.0f, 5.0f, -20.0f);
	float3 LightColour_Temp = float3(0.8f, 0.8f, 0.8f);
	float3 Ambient_Temp = float3(0.15f, 0.15f, 0.15f);
	float3 ObjectColour_Temp = float3(0.5f, 0.5f, 0.5f);

	float3 lightDir = normalize(LightPos_Temp - input.FragPos);
	
	float diffuse = max(0.0f, dot(lightDir, input.Normal));
	float3 diffuseResult = diffuse * LightColour_Temp;
	
	float3 shadingResult = (Ambient_Temp + diffuseResult) * ObjectColour_Temp;
	
    return float4(shadingResult, 1.0f);
}