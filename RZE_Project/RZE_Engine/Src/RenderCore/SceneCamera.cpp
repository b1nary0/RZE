#include <StdAfx.h>
#include <RenderCore/SceneCamera.h>

SceneCamera::SceneCamera(const SceneCameraProps& cameraProps)
{
	mCameraProps = cameraProps;
}

void SceneCamera::GenerateProjectionMat()
{
	mProjectionMat = Matrix4x4::CreatePerspectiveMatrix(mCameraProps.FOV, mCameraProps.AspectRatio, mCameraProps.NearCull, mCameraProps.FarCull);
}

void SceneCamera::GenerateViewMat()
{
	mViewMat = Matrix4x4::CreateViewMatrix(mCameraProps.Position, mCameraProps.Position + mCameraProps.FrontDir, mCameraProps.UpDir);
}

void SceneCamera::SetPosition(const Vector3D& newPos)
{
    mCameraProps.Position = newPos;
}

void SceneCamera::SetDirection(const Vector3D& newDir)
{
    mCameraProps.FrontDir = newDir;
}

const Vector3D& SceneCamera::GetPositionVec() const
{
	return mCameraProps.Position;
}

const Vector3D& SceneCamera::GetDirectionVec() const
{
	return mCameraProps.FrontDir;
}

const Vector3D& SceneCamera::GetUpVec() const
{
	return mCameraProps.UpDir;
}

const Matrix4x4& SceneCamera::GetProjectionMat() const
{
	return mProjectionMat;
}

const Matrix4x4& SceneCamera::GetViewMat() const
{
	return mViewMat;
}
