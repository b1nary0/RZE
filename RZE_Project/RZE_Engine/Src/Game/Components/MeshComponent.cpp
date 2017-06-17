#include <StdAfx.h>
#include <Game/Components/MeshComponent.h>

#include <RenderCore/HardwareInterface/OpenGL.h>

MeshComponent::MeshComponent()
{
    OpenGLRHI& RHI = OpenGLRHI::Get();

    mVAO = new OpenGLVAO();
    mVBO = new OpenGLVBO(mVAO);
}

MeshComponent::~MeshComponent()
{
    if (mVAO)
    {
        delete mVAO;
    }

    if (mVBO)
    {
        delete mVBO;
    }
}

std::vector<float>& MeshComponent::GetVertexList()
{
    return mVertexList;
}

void MeshComponent::SetVertexList(const std::vector<float>& vertexList)
{
    mVertexList = vertexList;
    //@note:josh this should be temporary, once more code is put in place
    //           to support mesh loading, this will end up happening at 
    //           the right time.
    mVAO->Bind();
    mVBO->SetBufferData(mVertexList.data(), sizeof(mVertexList.data()) * mVertexList.size());
    mVAO->Unbind();
}

GFXShaderGroup* const MeshComponent::GetShaderGroup()
{
    return mShaderGroup;
}

void MeshComponent::SetShaderGroup(GFXShaderGroup* const shaderGroup)
{
    mShaderGroup = shaderGroup;
}

OpenGLVAO* MeshComponent::GetVAO() const
{
    return mVAO;
}

OpenGLVBO* MeshComponent::GetVBO()
{
    return mVBO;
}
