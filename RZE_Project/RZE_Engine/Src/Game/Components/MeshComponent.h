#pragma once

#include <Game/ECS/EntityComponent.h>

class MeshData;
class GFXShaderGroup;
class OpenGLVAO;
class OpenGLVBO;

class MeshComponent : public IEntityComponent
{
public:
    MeshComponent();
    virtual ~MeshComponent();

    std::vector<float>& GetVertexList();
    void SetVertexList(const std::vector<float>& vertexList);

    // @todo this will be pulled out eventually and handled better.. or should be lol
    GFXShaderGroup* const GetShaderGroup();
    void SetShaderGroup(GFXShaderGroup* const shaderGroup);

    OpenGLVAO* GetVAO() const;
    OpenGLVBO* GetVBO();

private:
    OpenGLVAO* mVAO;
    OpenGLVBO* mVBO;

    // @todo:josh temp pointer for now because we have some cyclical issues in headers
    //            that needs to be worked out for now... 
    MeshData* meshData;

    GFXShaderGroup* mShaderGroup;
    std::vector<float> mVertexList;
};
