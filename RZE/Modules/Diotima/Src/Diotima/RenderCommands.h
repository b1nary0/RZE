#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Utils/Math/Matrix4x4.h>

namespace Diotima
{
	enum class ECreateBufferType
	{
		Invalid,
		Vertex,
		Index
	};

	enum class ECreateTextureBufferType
	{
		Invalid,
		Texture2D
	};

	struct CreateBufferRenderCommand
	{
		CreateBufferRenderCommand()
			: BufferType(ECreateBufferType::Invalid)
			, Data(nullptr)
			, NumElements(0) {}

		ECreateBufferType BufferType;
		void* Data;
		U32 NumElements;
	};

	struct CreateTextureBufferRenderCommand
	{
		CreateTextureBufferRenderCommand()
			: BufferType(ECreateTextureBufferType::Invalid)
			, Data(nullptr)
			, Width(0)
			, Height(0) {}

		ECreateTextureBufferType BufferType;
		void* Data;
		U32 Width;
		U32 Height;
	};

	struct UpdateRenderItemWorldMatrixCommand
	{
		U32 RenderItemID;
		Matrix4x4 WorldMtx;
	};

}