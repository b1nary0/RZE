#pragma once

#include <Utils/Platform/FilePath.h>

class SourceImporter
{
public:
	SourceImporter() = default;
	~SourceImporter() = default;

	void Import(const FilePath& filePath);

private:

};