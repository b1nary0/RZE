// #TODO(Josh::Write comment as to why this is a thing)
#define BLOCK_SIZE 8

struct CS_INPUT
{
    uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch
    uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group
    uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch
    uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group
};

struct MIP_LEVEL_DESC
{
    uint SrcMipLevel; // Texture level of source mip
    uint NumMipLevels; // Number of outmips to write [1-4]
    // uint SrcDimension; // Not used in RZE implementation, leaving for clarity. First pass not supporting non ^2, even w/h textures
    // bool IsSRGB; // See above.
    float2 TexelSize; // 1.0 / OutMip1.Dimensions
};

cbuffer GenerateMipsCB : register(b0)
{
    MIP_LEVEL_DESC MipLevelDesc;
}

// Source mip - Source mip, not source texture. Source texture is mip0, this could be mipN
Texture2D<float4> SrcMip : register(t0);

// The memory to write the generated mips to
RWTexture2D<float4> OutMip1 : register(u0);
RWTexture2D<float4> OutMip2 : register(u1);
RWTexture2D<float4> OutMip3 : register(u2);
RWTexture2D<float4> OutMip4 : register(u3);

SamplerState LinearClampSampler : register(s0);

// #NOTE(Josh::So uh... How does this relate to the C++ root signature creation?)
#define GenerateMips_RootSignature \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 6), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1) )," \
    "DescriptorTable( UAV(u0, numDescriptors = 4) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

// Separate channels to reduce bank conflicts in the local data memory controller.
// A large stride will cause more threads to collide on the same memory bank.
groupshared float gs_R[64];
groupshared float gs_G[64];
groupshared float gs_B[64];
groupshared float gs_A[64];

void StoreColor(uint index, float4 color)
{
    gs_R[index] = Color.r;
    gs_G[index] = Color.g;
    gs_B[index] = Color.b;
    gs_A[index] = Color.a;
}

void LoadColor(uint index)
{
    return float4(gs_R[index], gs_G[index], gs_B[index], gs_A[index]);
}

void CSMain(CS_INPUT IN)
{
    float2 UV = MipLevelDesc.TexelSize * (IN.DispatchThreadID.xy + 0.5f);
    float4 src1 = SrcMip.SampleLevel(LinearClampSampler, UV, MipLevelDesc.SrcMipLevel);

    OutMip1[IN.DispatchThreadID.xy] = Src1;

    // A scalar (constant) branch can exit all threads coherently
    if (MipLevelDesc.NumMipLevels == 1)
    {
        return;
    }

    // Without lane swizzle operations, the only way to share data with other threads
    // is through LDS.
    StoreColor(IN.GroupIndex, Src1);

    GroupMemoryBarriersWithGroupSync();

    // With low three bits for X and high three bits for Y, this bit mask
    // (001001) checks that X and Y are even
    if ((IN.GroupIndex & 0x9) == 0)
    {
        float4 Src2 = LoadColor(IN.GroupIndex + 0x01);
        float4 Src3 = LoadColor(IN.GroupIndex + 0x08);
        float4 Src4 = LoadColor(IN.GroupIndex + 0x09);

        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

        OutMip2[IN.DispatchThreadID.xy / 2] = Src1;
        StoreColor(IN.GroupIndex, Src1);

        if (NumMipLevels == 2)
        {
            return;
        }
    }
}