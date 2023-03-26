#pragma once

class AssetWriter
{
public:
	AssetWriter() = default;
	virtual ~AssetWriter() = default;

public:
	virtual void Process() = 0;
	virtual void Write() = 0;
};