#include <Diotima/Driver/GLRenderTarget.h>

#include <Diotima/Driver/OpenGL.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	GLRenderTargetTexture::GLRenderTargetTexture()
	{

	}

	GLRenderTargetTexture::~GLRenderTargetTexture()
	{

	}

	void GLRenderTargetTexture::Initialize()
	{
		glGenFramebuffers(1, &InternalGetFrameBufferID());
		glBindFramebuffer(GL_FRAMEBUFFER, GetFrameBufferID());

		glGenTextures(1, &InternalGetTextureID());
		glBindTexture(GL_TEXTURE_2D, GetTextureID());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GetWidth(), GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GetTextureID(), 0);
		
		glGenRenderbuffers(1, &InternalGetDepthBufferID());
		glBindRenderbuffer(GL_RENDERBUFFER, GetDepthBufferID());
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, GetWidth(), GetHeight());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GetDepthBufferID());
		glBindRenderbuffer(GL_RENDERBUFFER, 0);


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void GLRenderTargetTexture::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, GetFrameBufferID());
		glBindTexture(GL_TEXTURE_2D, GetTextureID());
	}

	void GLRenderTargetTexture::Unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

}