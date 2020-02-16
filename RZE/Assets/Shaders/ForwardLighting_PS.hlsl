
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

SamplerState s1 : register(s0);

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

struct MATERIAL_DATA
{
	float Shininess;
};

cbuffer LightConstantBuffer : register(b0, space1)
{
	LIGHT_INPUT_DESC lights[128];
};

cbuffer CameraRootConstant : register(b0, space2)
{
	CAMERA_INPUT_DESC cameraDesc;
};

cbuffer MaterialData : register(b0, space3)
{
	MATERIAL_DATA materialData;
};

cbuffer GlobalConstants : register(b0, space4)
{
	uint DirectionalLightCount;
	uint PointLightCount;
};

float CalculateBlinnPhong(float3 viewDir, float3 lightDir, float3 normal)
{
	float specular = 0.0f;
	
	// float3 halfDir = normalize(lightDir + viewDir);
	// float specAngle = max(0.0f, dot(halfDir, reflect(-lightDir, normal)));
	// specular = pow(specAngle, materialData.Shininess);

	float3 R = reflect(-lightDir, normal);
	float RdotV = max(0.0f, dot(R, viewDir)); 
	specular = pow(RdotV, materialData.Shininess);
	
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

	float bias = 0.001f;//max(0.05 * (1.0 - dot(normal, lightDir)), 0.25);
	float3 projectionCoords;
	projectionCoords.x = fragPosLightSpace.x / fragPosLightSpace.w / 2.0f + 0.5f;
	projectionCoords.y = fragPosLightSpace.y / fragPosLightSpace.w / 2.0f + 0.5f;

	if (saturate(projectionCoords.x) == projectionCoords.x && saturate(projectionCoords.y) == projectionCoords.y)
	{
		projectionCoords.y *= -1;
		float closestDepth = shadows.Sample(s1, projectionCoords.xy).r;
		float currentDepth = fragPosLightSpace.z / fragPosLightSpace.w;
		float shadowResult = currentDepth - bias > closestDepth ? 1.0 : 0.0;	
		return shadowResult;
	}
	
	return 0.0;
}

float4 PSMain(PS_IN input) : SV_TARGET
{
	float ambientCoeff = 0.45f;
	
	float3 normal = normalize(input.Normal);
	float3 tangent = normalize(input.Tangent);
	
	float4 diffSample = diffuse.Sample(s1, input.Color);
	float4 specularSample = specular.Sample(s1, input.Color);
	float4 bumpSample = bump.Sample(s1, input.Color);
	
	normal = CalculateBumpNormal(normal, tangent, bumpSample.rgb);
	
	float3 viewDir = normalize(cameraDesc.Position - input.FragPos);
	float3 ambientResult = ambientCoeff * diffSample.rgb;

	float3 lightAccum;
	uint lightIndex = 0;
	for (uint directionalLight = 0; directionalLight < DirectionalLightCount; ++directionalLight, ++lightIndex)
	{
		LIGHT_INPUT_DESC light = lights[lightIndex];

		float3 lightDir = normalize(light.Position.xyz - input.FragPos);
		
		float diff = max(0.0f, saturate(dot(normal, lightDir)));
		float specular = CalculateBlinnPhong(viewDir, lightDir, normal);
		
		float lightStrength = light.Strength;
		
		float3 diffuseResult = light.Color.rgb * lightStrength * diffSample.rgb;
		float3 specularResult = specular * lightStrength * light.Color.rgb * specularSample.rgb;
		
		float shadow = CalculateShadowFromDepthMap(light, input.FragPos, normal, lightDir);
		specularResult *= 1.0f - shadow;

		float3 result = (ambientResult + (diffuseResult * (1.0f - shadow))) + (specularResult * 0.5f);
		lightAccum += result;
	}

	for (uint pointLight = 0; pointLight < PointLightCount; ++pointLight, ++lightIndex)
	{
		LIGHT_INPUT_DESC light = lights[lightIndex];

		float3 lightDir = normalize(light.Position.xyz - input.FragPos);
		
		float diff = max(0.0f, saturate(dot(normal, lightDir)));
		float specular = CalculateBlinnPhong(viewDir, lightDir, normal);
		
		float lightStrength = CalculatePointLight(input.FragPos, light.Position.xyz, normal, viewDir, light.Strength);
		
		float3 diffuseResult = light.Color.rgb * lightStrength * diffSample.rgb;
		float3 specularResult = specular * lightStrength * light.Color.rgb;
		
		float3 result = (ambientResult + diffuseResult) + (specularResult * specularSample.rgb);
		lightAccum += result;
	}

    return float4(lightAccum, 1.0f);
}