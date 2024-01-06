#include <StdAfx.h>

#include <Rendering/RenderCommand.h>

#include <Rendering/Driver/DX11/DX11VertexBuffer.h>
#include <Rendering/Driver/DX11/DX11IndexBuffer.h>
#include <Rendering/Driver/DX11/DX11ConstantBuffer.h>

namespace Rendering
{
    void RenderCommand_CreateVertexBuffer::Execute()
    {
    }

    void RenderCommand_CreateIndexBuffer::Execute()
    {
    }

    void RenderCommand_CreateConstantBuffer::Execute()
    {
    }

    void RenderCommand_CreateTextureBuffer2D::Execute()
    {
    }

    void RenderCommand_CreateVertexShader::Execute()
    {
    }

    void RenderCommand_CreatePixelShader::Execute()
    {
    }

    void RenderCommand_UploadDataToBuffer::Execute()
    {
    }

    void RenderCommand_ReleaseVertexShader::Execute()
    {
        shaderHandle.m_shader.reset();
    }

    void RenderCommand_ReleasePixelShader::Execute()
    {
        shaderHandle.m_shader.reset();
    }

    void RenderCommand_ClearDepthStencilBuffer::Execute()
    {
    }
}