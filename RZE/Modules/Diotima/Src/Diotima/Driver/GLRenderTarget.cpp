#include <Diotima/Driver/GLRenderTarget.h>

#include <Diotima/Driver/OpenGL.h>

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
		glGenTextures(1, &InternalGetTextureID());

		glBindFramebuffer(GL_FRAMEBUFFER, GetFrameBufferID());
		glBindTexture(GL_TEXTURE_2D, GetTextureID());

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GetWidth(), GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glGenRenderbuffers(1, &InternalGetDepthBufferID());
		glBindRenderbuffer(GL_RENDERBUFFER, GetDepthBufferID());
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, GetWidth(), GetHeight());

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, GetDepthBufferID());
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GetTextureID(), 0);

		glBindTexture(GL_TEXTURE_2D, 0);
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