#pragma once

#include <Utils/Interfaces/Resource.h>

class Texture2D : public IResource
{
public:
	Texture2D();
	~Texture2D();

	virtual bool Load(const FilePath& filePath) final override;
	virtual void Release() final override;
};