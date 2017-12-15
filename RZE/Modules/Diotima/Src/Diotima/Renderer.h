#pragma once

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Diotima/SceneCamera.h>
#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Driver/GLRenderTarget.h>
#include <Diotima/Graphics/Material.h>

#include <Utils/Interfaces/SubSystem.h>
#include <Utils/Math/Vector2D.h>

namespace Diotima
{
	class MeshResource;
	class GFXMaterial;
	class GFXShaderGroup;
	class GFXTexture2D;
	
	class Renderer : public ISubSystem
	{
	public:
		typedef struct RenderItemProtocol
		{
			RenderItemProtocol();

			GFXShaderGroup*		Shader { nullptr };
			GFXMaterial			Material;
			GFXTexture2D*		Texture2D { nullptr };
			Matrix4x4           ModelMat;
			Matrix4x4           ProjectionMat;
			Matrix4x4           ViewMat;
			MeshResource*       MeshData { nullptr };
		} RenderItemProtocol;

		typedef struct LightItemProtocol
		{
			Vector3D	Position;
			Vector3D	Color;
			
			float		Strength;
		} LightItemProtocol;

		typedef struct CameraItemProtocol
		{
			Vector3D Position;
			Matrix4x4 ProjectionMat;
			Matrix4x4 ViewMat;

			float FOV;
			float AspectRatio;
			float NearCull;
			float FarCull;
		} CameraItemProtocol;

		// Constructors
	public:
		// #TODO(Josh) Remove the parameters here and work in a better event driven system?
		Renderer();

		// ISubSystem interface
	public:
		virtual void Initialize();
		virtual void Update();
		virtual void ShutDown();

	public:
		void AddRenderItem(const RenderItemProtocol& itemProtocol);
		void AddLightItem(const LightItemProtocol& itemProtocol);

		void SetCamera(const CameraItemProtocol& cameraItem) { camera = std::move(cameraItem); }

		void ResizeCanvas(const Vector2D& newSize);

		void ClearLists();
		
	private:
		void RenderSingleItem(RenderItemProtocol& itemProtocol);

		void RenderToTexture_Test(RenderItemProtocol& itemProtocol);

	private:
		GLRenderTargetTexture mRenderTargetTexture;

		CameraItemProtocol camera;
		std::queue<RenderItemProtocol> mRenderList;
		std::vector<LightItemProtocol> mLightingList;
	};
}
