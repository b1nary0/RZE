#include <Diotima/Driver/GLRenderTarget.h>

#include <Diotima/Driver/OpenGL.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	GLRenderTargetTexture::GLRenderTargetTexture()
	{
		const OpenGLRHI& openGL = OpenGLRHI::Get();

		openGL.GenerateFrameBuffer(1, &InternalGetFrameBufferID());
		openGL.GenerateTexture(1, &InternalGetTextureID());
		openGL.GenerateRenderBuffer(1, &InternalGetDepthBufferID());
	}

	GLRenderTargetTexture::~GLRenderTargetTexture()
	{

	}

	void GLRenderTargetTexture::Initialize()
	{
		const OpenGLRHI& openGL = OpenGLRHI::Get();

		openGL.BindFramebuffer(EGLBufferTarget::FrameBuffer, GetFrameBufferID());
		openGL.BindTexture(EGLTextureTarget::Texture2DMultisample, GetTextureID());
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 16, GL_RGB8, GetWidth(), GetHeight(), true);
		// #TODO(Josh) :: The GL_RGB enum needs to be defined in OpenGL.h
		//openGL.TextureImage2D(EGLTextureTarget::Texture2D, 0, GL_RGB, GetWidth(), GetHeight(), 0, GL_RGB, EGLDataType::UnsignedByte, 0);
		//openGL.SetTextureParami(EGLTextureTarget::Texture2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//openGL.SetTextureParami(EGLTextureTarget::Texture2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//openGL.BindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		openGL.SetFramebufferTexture2D(EGLBufferTarget::FrameBuffer, EGLAttachmentPoint::Color0, EGLTextureTarget::Texture2DMultisample, GetTextureID(), 0);
		
		openGL.BindRenderbuffer(GetDepthBufferID());
		openGL.AllocateRenderbufferStorage(GL_DEPTH_COMPONENT16, GetWidth(), GetHeight());
		openGL.AttachRenderBufferToFrameBuffer(EGLAttachmentPoint::Depth, GetDepthBufferID());
		openGL.BindRenderbuffer(0);


		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		LOG_CONSOLE_ARGS("FrameBufferComplete status for FBO %i : %i", GetFrameBufferID(), status);
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
	

	/////////////////////////////////
	////////////////////////////////


	GLRenderTargetDepthTexture::GLRenderTargetDepthTexture()
	{
		const OpenGLRHI& openGL = OpenGLRHI::Get();
		openGL.GenerateFrameBuffer(1, &InternalGetFrameBufferID());
		openGL.GenerateTexture(1, &InternalGetTextureID());
	}

	GLRenderTargetDepthTexture::~GLRenderTargetDepthTexture()
	{
	}

	void GLRenderTargetDepthTexture::Initialize()
	{
		const OpenGLRHI& openGL = OpenGLRHI::Get();
		openGL.BindFramebuffer(EGLBufferTarget::FrameBuffer, GetFrameBufferID());
		openGL.BindTexture(GL_TEXTURE_2D, GetTextureID());
		openGL.TextureImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GetWidth(), GetHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		openGL.SetTextureParami(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		openGL.SetTextureParami(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		openGL.SetTextureParami(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		openGL.SetTextureParami(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		openGL.SetFramebufferTexture2D(EGLBufferTarget::FrameBuffer, EGLAttachmentPoint::Depth, EGLTextureTarget::Texture2D, GetTextureID(), 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		openGL.BindFramebuffer(EGLBufferTarget::FrameBuffer, 0);
	}

}