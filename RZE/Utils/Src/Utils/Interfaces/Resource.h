#pragma once

#include <Utils/Platform/FilePath.h>

class IResource
{
public:
	IResource() = default;
	virtual ~IResource() = default;

	virtual bool Load(const FilePath& filePath) = 0;
	virtual void Release() = 0;

	virtual bool Load(const U8* buffer, int width, int height) { return false; }

protected:
};
