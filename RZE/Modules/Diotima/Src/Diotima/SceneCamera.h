#pragma once

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

namespace Diotima
{
	typedef struct SceneCameraProps
	{
		Vector3D Position;
		Vector3D Direction;
		Vector3D UpDir;
		Vector3D FrontDir;

		float FOV;
		float AspectRatio;
		float NearCull;
		float FarCull;
	} SceneCameraProps;

	class SceneCamera
	{
	public:
		SceneCamera() = delete;
		SceneCamera(const SceneCameraProps& cameraProps);

		void GenerateProjectionMat();
		void GenerateViewMat();

		void SetPosition(const Vector3D& newPos);
		void SetDirection(const Vector3D& newDir);

		const Vector3D& GetPositionVec();
		const Vector3D& GetDirectionVec();
		const Vector3D& GetUpVec();

		const Matrix4x4& GetProjectionMat();
		const Matrix4x4& GetViewMat();

	private:
		SceneCameraProps mCameraProps;

		Matrix4x4 mProjectionMat;
		Matrix4x4 mViewMat;
	};
}
