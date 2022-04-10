#pragma once

#include <Utils/Platform/Filepath.h>

class IResource
{
public:
	IResource() = default;
	virtual ~IResource() = default;

	virtual bool Load(const Filepath& filePath) = 0;
	virtual void Release() = 0;

	virtual bool Load(const U8* buffer, int width, int height) { return false; }

protected:
};
