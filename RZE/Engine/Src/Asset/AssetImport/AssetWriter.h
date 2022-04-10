#pragma once

class AssetWriter
{
public:
	AssetWriter() = default;
	virtual ~AssetWriter() = default;

public:
	virtual void Write() = 0;
};