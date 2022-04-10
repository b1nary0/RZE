#pragma once

#include <Utils/Platform/Filepath.h>

class SourceImporter
{
public:
	SourceImporter() = default;
	virtual ~SourceImporter() = default;
	
	virtual bool Import(const Filepath& filePath) = 0;

private:

};