#pragma once

#include <Rendering/BufferHandle.h>

#include <Utils/PrimitiveDefs.h>

#include <Utils/Interfaces/Resource.h>

class Texture2D : public IResource
{
public:
	Texture2D();
	virtual ~Texture2D();

public:
	bool Load(const FilePath& filePath) final;
	void Release() final;

public:
	U8* GetRawData() const { return m_data; }
	Vector2D GetDimensions() const;

	Rendering::TextureBuffer2DHandle GetPlatformObject() const { return m_GPUResource; }

private:
	U8* m_data;

	Int32 m_width;
	Int32 m_height;
	Int32 m_channels;

	Rendering::TextureBuffer2DHandle m_GPUResource;
};