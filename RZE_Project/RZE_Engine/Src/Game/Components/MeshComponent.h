#pragma once

#include <Game/ECS/EntityComponent.h>

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

    GFXShaderGroup* const GetShaderGroup();
    void SetShaderGroup(GFXShaderGroup* const shaderGroup);

    OpenGLVAO* GetVAO() const;
    OpenGLVBO* GetVBO();

private:
    OpenGLVAO* mVAO;
    OpenGLVBO* mVBO;

    GFXShaderGroup* mShaderGroup;
    std::vector<float> mVertexList;
};
