#pragma once

#include <EngineCore/Resources/ResourceHandler.h>

#include <Game/ECS/EntityComponent.h>

#include <RenderCore/HardwareInterface/OpenGL.h>

class GFXShaderGroup;
class GFXTexture2D;

class MeshComponent : public IEntityComponent
{
public:
    MeshComponent();
    MeshComponent(const std::string& name);
    virtual ~MeshComponent();

    std::vector<float>& GetVertexList();

    // @todo this will be pulled out eventually and handled better.. or should be lol
    GFXShaderGroup* const GetShaderGroup();
    void SetShaderGroup(GFXShaderGroup* const shaderGroup);

    const ResourceHandle& GetMeshHandle() const;
    void SetMeshHandle(const ResourceHandle& resourceHandle);

    const ResourceHandle& GetTextureHandle() const;
    void SetTextureHandle(const ResourceHandle& resourceHandle);

private:

    // @todo:josh temp pointer for now because we have some cyclical issues in headers
    //            that needs to be worked out for now... 
    ResourceHandle mMeshHandle;
    // #TODO this will have to be abstracted better or some other way to deal with giving a mesh/model a texture period and
    //       then offloading the actual type (1D, 2D, 3D) later.
    ResourceHandle mTextureHandle;

    GFXShaderGroup* mShaderGroup;

    std::vector<float> mVertexList;
};
