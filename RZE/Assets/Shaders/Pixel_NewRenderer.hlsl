struct PS_IN
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UVCoords : UV;
	float3 Tangent : TANGENT;
	float3 FragPos : POSITION;
	float3 CameraPos : POSITION1;
};

struct MATERIAL_DATA
{
	float Shininess;
	float Opacity;
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

float3 CalculateBumpNormal(float3 inNormal, float3 inTangent, float3 inBumpMapSample)
{
	float3 tangent = inTangent;
	tangent = normalize(tangent - dot(tangent, inNormal) * inNormal);
	
	float3 biTangent = cross(tangent, inNormal);
	float3 bumpMapNormal = (2.0f * inBumpMapSample) - 1.0f;
	
	float3x3 TBN = float3x3(tangent, biTangent, inNormal);
	float3 newNormal = normalize(mul(bumpMapNormal, TBN));
	
	return newNormal;
}

float CalculateBlinnPhong(float3 viewDir, float3 lightDir, float3 normal)
{
	float specular = 0.0f;
	
	 float3 halfDir = normalize(lightDir + viewDir);
	 float specAngle = max(0.0f, dot(halfDir, normal));
	 specular = pow(specAngle, MaterialData.Shininess);
	
	return specular;
}

float4 PSMain(PS_IN input) : SV_TARGET
{
	float minDiffuseFactor = 0.5f;
	
	float3 Ambient_Temp = float3(0.1f, 0.1f, 0.1f);

	float3 lightDir = lightData.position - input.FragPos;
	float distance = length(lightDir);
	lightDir = normalize(lightDir);
	
	float3 viewDir = normalize(input.CameraPos - input.FragPos);
	
	float3 shadingResult;
	{
		float4 diffSample = textures[0].Sample(samplerState, input.UVCoords);
		float4 specularSample = textures[1].Sample(samplerState, input.UVCoords);
		float4 bumpSample = textures[2].Sample(samplerState, input.UVCoords);
		
		float3 bumpNormal = CalculateBumpNormal(input.Normal, input.Tangent, bumpSample.rgb);
		
		float diffuse = max(dot(bumpNormal, lightDir), minDiffuseFactor);
		float3 diffuseResult = saturate(diffuse * diffSample.rgb * lightData.colour);
		
		float specularValue = CalculateBlinnPhong(viewDir, lightDir, bumpNormal);
		float3 specularResult = saturate(specularSample.rgb * specularValue * diffuseResult.rgb);
		
		shadingResult = (Ambient_Temp + diffuseResult + specularResult);
	}
	
	
    return float4(shadingResult, 1.0f);
}