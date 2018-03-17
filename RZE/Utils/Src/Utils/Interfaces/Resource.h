#pragma once

class IResource
{
public:
	IResource() = default;
	virtual ~IResource() = default;

	virtual bool Load(const std::string& filePath) = 0;
	virtual void Release() = 0;

protected:
};
