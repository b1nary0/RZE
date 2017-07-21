#pragma once

#include <EngineCore/Resources/ResourceHandler.h>

#include <Game/ECS/EntityComponent.h>

#include <RenderCore/Graphics/Mesh.h>

class GFXShaderGroup;
class OpenGLVAO;
class OpenGLVBO;
class OpenGLEBO;

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

    const ResourceHandle& GetResource() const;
    void SetResource(const ResourceHandle& resourceHandle);

private:

    // @todo:josh temp pointer for now because we have some cyclical issues in headers
    //            that needs to be worked out for now... 
    ResourceHandle mResourceHandle;

    GFXShaderGroup* mShaderGroup;
    std::vector<float> mVertexList;
};
