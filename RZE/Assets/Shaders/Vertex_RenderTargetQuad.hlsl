struct VS_OUT
{
	float4 Position : SV_POSITION;
	float2 UV       : UV;
};

// Fullscreen quad generated from the vertex ID; no vertex buffer needed
VS_OUT VSMain(uint vertexID : SV_VERTEXID)
{
	VS_OUT output;

	float2 uv = float2(vertexID & 1, vertexID >> 1);

	output.UV = uv;
	output.Position = float4((uv.x - 0.5f) * 2.0f, -(uv.y - 0.5f) * 2.0f, 0.0f, 1.0f);

	return output;
}
