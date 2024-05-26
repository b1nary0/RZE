struct VS_IN
{
	float3 Position : POSITION;
	float3 Colour : COLOUR;
};

struct VS_OUT
{
	float4 Position : SV_POSITION;
	float3 Colour : COLOUR;
};

struct CAMERA_INPUT_DATA
{
	matrix ClipSpace;
	float3 Position;
};

cbuffer CameraDataBuf : register(b0)
{
	CAMERA_INPUT_DATA CameraData;
};

cbuffer ModelMatBuf : register(b1)
{
	matrix ModelView;
	//matrix InvModelView;
};

VS_OUT VSMain(VS_IN input) // main is the default function name
{
	VS_OUT output;
	
	matrix mvp = mul(CameraData.ClipSpace, ModelView);
	
	output.Position = mul(mvp, float4(input.Position, 1.0f));
	output.Colour = float4(255.0f, 0.0f, 0.0f, 1.0f);
	
    return output;
}