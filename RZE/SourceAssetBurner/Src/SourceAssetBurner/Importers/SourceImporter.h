#pragma once

#include <Utils/Platform/FilePath.h>

class SourceImporter
{
public:
	SourceImporter() = default;
	virtual ~SourceImporter() = default;
	
	virtual bool Import(const FilePath& filePath) = 0;

private:

};