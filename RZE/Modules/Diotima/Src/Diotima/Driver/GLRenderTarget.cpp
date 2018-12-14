#include <Diotima/Driver/GLRenderTarget.h>

#include <Diotima/Driver/OpenGL.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	GLRenderTargetTexture::GLRenderTargetTexture()
	{
		OpenGLRHI& openGL = OpenGLRHI::Get();

		openGL.GenerateFrameBuffer(1, &InternalGetFrameBufferID());
		openGL.GenerateTexture(1, &InternalGetTextureID());
		openGL.GenerateRenderBuffer(1, &InternalGetDepthBufferID());
	}

	GLRenderTargetTexture::~GLRenderTargetTexture()
	{

	}

	void GLRenderTargetTexture::Initialize()
	{
		OpenGLRHI& openGL = OpenGLRHI::Get();

		openGL.BindFramebuffer(EGLBufferTarget::FrameBuffer, GetFrameBufferID());
		openGL.BindTexture(EGLTextureTarget::Texture2D, GetTextureID());
		// #TODO(Josh) :: The GL_RGB enum needs to be defined in OpenGL.h
		openGL.TextureImage2D(EGLTextureTarget::Texture2D, 0, GL_RGB, GetWidth(), GetHeight(), 0, GL_RGB, EGLDataType::UnsignedByte, 0);
		openGL.SetTextureParami(EGLTextureTarget::Texture2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		openGL.SetTextureParami(EGLTextureTarget::Texture2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		openGL.BindTexture(GL_TEXTURE_2D, 0);
		openGL.SetFramebufferTexture2D(EGLBufferTarget::FrameBuffer, EGLAttachmentPoint::Color0, EGLTextureTarget::Texture2D, GetTextureID(), 0);
		
		openGL.BindRenderbuffer(GetDepthBufferID());
		openGL.AllocateRenderbufferStorage(GL_DEPTH24_STENCIL8, GetWidth(), GetHeight());
		openGL.AttachRenderBufferToFrameBuffer(EGLAttachmentPoint::DepthStencil, GetDepthBufferID());
		openGL.BindRenderbuffer(0);


		openGL.BindFramebuffer(EGLBufferTarget::FrameBuffer, 0);
	}

	void GLRenderTargetTexture::Bind()
	{
		OpenGLRHI::Get().BindFramebuffer(EGLBufferTarget::FrameBuffer, GetFrameBufferID());
	}

	void GLRenderTargetTexture::Unbind()
	{
		OpenGLRHI::Get().BindFramebuffer(EGLBufferTarget::FrameBuffer, 0);
	}

}