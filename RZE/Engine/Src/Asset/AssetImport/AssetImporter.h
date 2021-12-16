#pragma once

class AssetImporter
{
public:
	AssetImporter() = default;
	virtual ~AssetImporter() = default;

public:
	virtual bool Import(const FilePath& filePath) = 0;
};