#pragma once

#include <Utils/PrimitiveDefs.h>

// #TODO(Josh::Potentially temporary place for now until I figure out a better one.
//             Also don't like how this is in a DX12 marked file but it's too late now. Fix later.)
struct CBAllocationData
{
	U64 GPUBaseAddr;
	size_t SizeInBytes;
};