#pragma once
#include <RZE_Config.h>

#include <vector>

class VertexBuffer
{
public:
	VertexBuffer(const std::vector<float>& localBuffer);
	~VertexBuffer();

	void Initialize();

	inline const std::vector<float>& GetData() const { return mLocalBuffer; }

private:
	// The CPU buffer for the mesh data
	std::vector<float> mLocalBuffer;
};
