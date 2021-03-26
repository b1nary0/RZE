#pragma once

#include <RZE_Config.h>

#include <vector>

#include <Utils/PrimitiveDefs.h>

class IndexBuffer
{
public:
	IndexBuffer(const std::vector<U32>& indexData);
	~IndexBuffer() = default;

public:
	void Initialize();

	inline const std::vector<U32>& GetData() const { return mLocalBuffer; }

private:
	std::vector<U32> mLocalBuffer;
};
