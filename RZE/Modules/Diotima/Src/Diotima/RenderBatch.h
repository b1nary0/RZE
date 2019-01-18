#pragma once

#include <vector>

#include <Diotima/Driver/OpenGL/OpenGL.h>
namespace Diotima
{
	class GFXMesh;

	class RenderBatch
	{
	public:
		RenderBatch();

		void Allocate(const std::vector<GFXMesh*>& meshes);

	public:
		OpenGLVAO mVAO;
		OpenGLVBO mVertexVBO;
		OpenGLEBO mEBO;

		U32 mNumIndices;
	};
}