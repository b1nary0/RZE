#pragma once

class IResource
{
public:
	IResource() {}
	virtual ~IResource() {}
	virtual bool Load(const std::string& filePath) = 0;
	virtual void Release() = 0;

protected:
};
