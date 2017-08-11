#pragma once

#include <vector>

#include <RenderCore/SceneCamera.h>
#include <RenderCore/HardwareInterface/OpenGL.h>

class MeshResource;
class GFXShaderGroup;
class GFXTexture2D;

class RZE_Renderer
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

public:
    RZE_Renderer();

    void AddRenderItem(const RenderItemProtocol& itemProtocol);
    void AddLightItem(const LightItemProtocol& itemProtocol);

    void Render();

    SceneCamera& GetSceneCamera();

private:
    void RenderSingleItem(RenderItemProtocol& itemProtocol);

    void RenderUI();

private:
    SceneCamera* mSceneCamera;

    std::queue<RenderItemProtocol> mRenderList;
    std::vector<LightItemProtocol> mLightingList;
};
