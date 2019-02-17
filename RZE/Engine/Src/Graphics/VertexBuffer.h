#pragma once

#include <vector>

class VertexBuffer
{
public:
	VertexBuffer(const std::vector<float>& localBuffer);
	~VertexBuffer();

	void Initialize();

private:
	// The CPU buffer for the mesh data
	std::vector<float> mLocalBuffer;
};