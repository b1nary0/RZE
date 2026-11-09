#pragma once

class AssetWriter
{
public:
	AssetWriter() = default;
	virtual ~AssetWriter() = default;

public:
	// True once the asset's file is written. On false the writer has logged why, and any file already there is kept.
	virtual bool Write() = 0;
};