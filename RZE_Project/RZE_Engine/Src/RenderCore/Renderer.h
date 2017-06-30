#pragma once

#include <vector>

#include <RenderCore/HardwareInterface/OpenGL.h>
#include <RenderCore/SceneCamera.h>

class MeshData;
class GFXShaderGroup;

class RZE_Renderer
{
public:
    typedef struct RenderItemProtocol
    {
        GFXShaderGroup* mShaderGroup;
        Matrix4x4 mModelViewProjection;
        MeshData* mMeshData;
    } RenderItemProtocol;

public:
    RZE_Renderer();

    void AddRenderItem(const RenderItemProtocol& itemProtocol);
    void Render();

    SceneCamera& GetSceneCamera();

private:
    void RenderSingleItem(RenderItemProtocol& itemProtocol);

private:
    SceneCamera* mSceneCamera;

    std::queue<RenderItemProtocol> mRenderList;
};
