
struct PS_IN
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 Color : UV;
	float3 Tangent : TANGENT;
	float3 FragPos : POSITION;
};

Texture2D diffuse : register(t0);
Texture2D specular : register(t1);
Texture2D bump : register(t2);
Texture2D shadows : register(t3);

SamplerState diffSampler : register(s0);
SamplerState specSampler : register(s1);
SamplerState bumpSampler : register(s2);

struct LIGHT_INPUT_DESC
{
	float4 Position;
	float4 Color;
	matrix LightSpaceMat;
	float1 Strength;
	uint PlaceHolder;
	float2 Padding;
};

struct CAMERA_INPUT_DESC
{
	float3 Position;
};

struct MATERIAL_INPUT_DESC
{
	bool IsTextured;
	float Shininess;
};

cbuffer LightConstantBuffer : register(b0)
{
	LIGHT_INPUT_DESC sceneLight;
};

cbuffer CameraRootConstant : register(b1)
{
	CAMERA_INPUT_DESC cameraDesc;
};

cbuffer MaterialConstantBuffer : register(b2)
{
	MATERIAL_INPUT_DESC materialData;
};

float CalculateBlinnPhong(float3 viewDir, float3 lightDir, float3 normal)
{
	float specular = 0.0f;
	
	 float3 halfDir = normalize(lightDir + viewDir);
	 float specAngle = max(0.0f, dot(halfDir, reflect(-lightDir, normal)));
	 specular = pow(specAngle, materialData.Shininess);

	//float3 R = reflect(-lightDir, normal);
	//float RdotV = max(0.0f, dot(R, viewDir)); 
	//specular = pow(RdotV, materialData.Shininess);
	
	return specular;
}

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

float CalcAttenuation(float dist, float falloffStart, float falloffEnd)
{
	return saturate((falloffEnd - dist) / (falloffEnd - falloffStart));
}

float CalculatePointLight(float3 pixelPos, float3 lightPos, float3 normal, float3 toEye, float1 strength)
{
	float tempFalloffStart = 4.0f;
	float tempFalloffEnd = 16.0f;
	
	float3 lightVec = lightPos - pixelPos;
	float distance = length(lightVec);
	
	if (distance > tempFalloffEnd)
	{
		return 0.0f;
	}
	
	lightVec = normalize(lightVec);
	
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float lightStrength = strength * ndotl;
	
	float att = CalcAttenuation(distance, tempFalloffStart, tempFalloffEnd);
	lightStrength *= att;
	
	return lightStrength;
}

float CalculateShadowFromDepthMap(LIGHT_INPUT_DESC light, float3 fragPos, float3 normal, float3 lightDir)
{
	float4 fragPosLightSpace = mul(light.LightSpaceMat, float4(fragPos, 1.0f));

	float bias = 0.001f;//max(0.05 * (1.0 - dot(lightDir, normal)), 0.25);
	float3 projectionCoords;
	projectionCoords.x = fragPosLightSpace.x / fragPosLightSpace.w / 2.0f + 0.5f;
	projectionCoords.y = fragPosLightSpace.y / fragPosLightSpace.w / 2.0f + 0.5f;

	if (saturate(projectionCoords.x) == projectionCoords.x && saturate(projectionCoords.y) == projectionCoords.y)
	{
		projectionCoords.y *= -1;
		float closestDepth = shadows.Sample(diffSampler, projectionCoords.xy).r;
		float currentDepth = fragPosLightSpace.z / fragPosLightSpace.w;
		float shadowResult = currentDepth - bias > closestDepth ? 1.0 : 0.0;	
		return shadowResult;
	}
	
	return 0.0;
}

float4 PSMain(PS_IN input) : SV_TARGET
{
	float ambientCoeff = 0.5f;
	
	float3 normal = normalize(input.Normal);
	float3 tangent = normalize(input.Tangent);
	
	float3 lightDir = normalize(input.FragPos - sceneLight.Position.xyz);
	float3 viewDir = normalize(input.FragPos - cameraDesc.Position);
	
	float3 ambientResult = ambientCoeff * float3(1.0f, 1.0f, 1.0f);
	float3 specularResult = float3(1.0f, 1.0f, 1.0f);
	float3 diffuseResult = float3(0.25f, 0.25f, 0.25f);
		
	if (materialData.IsTextured)
	{
		float4 diffSample = diffuse.Sample(diffSampler, input.Color);
		float4 specularSample = specular.Sample(specSampler, input.Color);
		float4 bumpSample = bump.Sample(bumpSampler, input.Color);
	
		normal = CalculateBumpNormal(normal, tangent, bumpSample.rgb);
	
		float specularValue = CalculateBlinnPhong(viewDir, lightDir, normal);
		specularResult = specularResult * specularSample.rgb * specularValue;
		diffuseResult = diffSample * diffSample.rgb;
		ambientResult = ambientResult * diffSample.rgb;
	}
	else
	{
		// What is this meant to be? Seems to have been forgotten..
		//float diff = max(0.0f, saturate(dot(lightDir, lightDir)));
		//diffuseResult = diffuseResult * diff;
		
		// Done here because of availability of mesh normal vs bump normal
		// in current code
		float specularValue = CalculateBlinnPhong(viewDir, lightDir, normal);
		specularResult = specularResult * specularValue;
	}
	
	float shadow = CalculateShadowFromDepthMap(sceneLight, input.FragPos, normal, lightDir);
	specularResult *= 1.0f - shadow;

	diffuseResult = diffuseResult * sceneLight.Color * sceneLight.Strength;
	specularResult = specularResult * sceneLight.Strength * sceneLight.Color;
	
	float3 result = (ambientResult + (diffuseResult * (1.0f - shadow))) + (specularResult * 0.5f);
	float4 realResult = float4(result, 1.0f);

    return realResult;
}