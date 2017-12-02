#pragma once

#include <vector>

#include <Diotima/SceneCamera.h>
#include <Diotima/Driver/OpenGL.h>

#include <Utils/Interfaces/SubSystem.h>
#include <Utils/Math/Vector2D.h>

namespace Diotima
{
	class MeshResource;
	class GFXShaderGroup;
	class GFXTexture2D;
	class GFXFont;

	class RenderSystem : public ISubSystem
	{
	public:
		typedef struct RenderItemProtocol
		{
			RenderItemProtocol();

			GFXShaderGroup*     mShaderGroup;
			Matrix4x4           mModelMat;
			Matrix4x4           mProjectionMat;
			Matrix4x4           mViewMat;
			MeshResource*       mMeshData;
			GFXTexture2D*       mTextureData;
		} RenderItemProtocol;

		typedef struct LightItemProtocol
		{
			Vector3D            mLightColor;
			Vector3D            mLightPos;
			float               mLightStrength;
		} LightItemProtocol;

		typedef struct FontItemProtocol
		{
			GFXFont*        mFont;
			Matrix4x4       mProjectionMat;
			Vector3D		mPosition;
			std::string     mText;
			GFXShaderGroup* mShaderGroup;
		} FontItemProtocol;

	// Constructors
	public:
		// #TODO(Josh) Remove the parameters here and work in a better event driven system?
		RenderSystem();

	// ISubSystem interface
	public:
		virtual void Initialize();
		virtual void Update();
		virtual void ShutDown();

	public:
		void AddRenderItem(const RenderItemProtocol& itemProtocol);
		void AddLightItem(const LightItemProtocol& itemProtocol);

		void ClearLists();

		void ResizeCanvas(const Vector2D& newSize);

		SceneCamera& GetSceneCamera();

	private:
		void RenderSingleItem(RenderItemProtocol& itemProtocol);

	private:
		SceneCamera* mSceneCamera;

		std::vector<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;
		std::vector<FontItemProtocol>  mFontList;
	};
}
