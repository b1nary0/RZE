#pragma once

#include <vector>
#include <queue>

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

			GFXShaderGroup*     ShaderGroup;
			Matrix4x4           ModelMat;
			Matrix4x4           ProjectionMat;
			Matrix4x4           ViewMat;
			MeshResource*       MeshData;
			GFXTexture2D*       TextureData;
		} RenderItemProtocol;

		typedef struct LightItemProtocol
		{
			Vector3D            LightColor;
			Vector3D            LightPos;
			float               LightStrength;
		} LightItemProtocol;

		typedef struct FontItemProtocol
		{
			GFXFont*        Font;
			Matrix4x4       ProjectionMat;
			Vector3D		Position;
			std::string     Text;
			GFXShaderGroup* ShaderGroup;
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

		std::queue<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;
		std::vector<FontItemProtocol>  mFontList;
	};
}
