#pragma once

#include <Utils/Platform/Filepath.h>

class AssetImporter
{
public:
	AssetImporter() = default;
	virtual ~AssetImporter() = default;

public:
	virtual bool Import(const Filepath& filePath) = 0;
};