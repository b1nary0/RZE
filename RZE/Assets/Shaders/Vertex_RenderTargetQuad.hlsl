struct VS_OUT
{
	float4 Position : SV_POSITION;
	float2 UVCoords : UV;
};

VS_OUT VSMain(uint vI : SV_VERTEXID)
{
	VS_OUT output;
	
    float2 texcoord = float2(vI&1,vI>>1);
	
	output.UVCoords = texcoord;
	output.Position = float4((texcoord.x-0.5f)*2,-(texcoord.y-0.5f)*2,0,1);
	
    return output;
}