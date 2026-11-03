#include <Rendering/Driver/OpenGL/GLDevice.h>

#include <Rendering/Driver/OpenGL/GLCommon.h>
#include <Rendering/Driver/OpenGL/GLConstantBuffer.h>
#include <Rendering/Driver/OpenGL/GLIndexBuffer.h>
#include <Rendering/Driver/OpenGL/GLShaderTypes.h>
#include <Rendering/Driver/OpenGL/GLTextureBuffer2D.h>
#include <Rendering/Driver/OpenGL/GLVertexBuffer.h>
#include <Rendering/TexelReadback.h>
#include <Rendering/TextureReadback.h>

#include <Utils/Math/Vector4D.h>

#include <SDL3/SDL.h>

#include <string>

namespace Rendering
{
	namespace
	{
		// As DX11Device::SetRenderTargetBackBuffer clears to
		constexpr GLfloat k_backBufferClearColour[4] = { 0.25f, 0.45f, 0.65f, 1.0f };

		U32 GetBytesPerTexel(const GFXTextureBufferParams& params)
		{
			if (params.bIsDepthTexture)
			{
				return 4;
			}

			switch (params.Format)
			{
			case ETextureFormat::R8_UNORM:
				return 1;
			case ETextureFormat::R16_FLOAT:
				return 2;
			case ETextureFormat::RGBA16_FLOAT:
			case ETextureFormat::RG32_UINT:
				return 8;
			case ETextureFormat::RGBA8_UNORM:
			case ETextureFormat::RG16_FLOAT:
			case ETextureFormat::R32_UINT:
			case ETextureFormat::R32_FLOAT:
			default:
				return 4;
			}
		}

#ifdef _DEBUG
		// KHR_debug's reports, which a debug context gives as they happen, on the thread that made the call
		void GLAPIENTRY ReportDebugMessage(GLenum /*source*/, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
		{
			if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
			{
				return;
			}

			RZE_LOG_ERROR("OpenGL (" << id << "): " << message);
			AssertMsg(type != GL_DEBUG_TYPE_ERROR, "An OpenGL call failed (see the log)");
		}
#endif
	}

	void GLDevice::SetWindow(void* windowHandle)
	{
		m_window = static_cast<SDL_Window*>(windowHandle);
	}

	void GLDevice::Initialize()
	{
		AssertNotNull(m_window);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef _DEBUG
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

		// Current on this thread until DetachFromCurrentThread
		m_context = SDL_GL_CreateContext(m_window);
		if (m_context == nullptr)
		{
			RZE_LOG_ERROR("Couldn't make an OpenGL 4.5 context: " << SDL_GetError());
			AssertFalse();
			return;
		}

		const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
		if (GLAD_VERSION_MAJOR(version) < 4 || (GLAD_VERSION_MAJOR(version) == 4 && GLAD_VERSION_MINOR(version) < 5))
		{
			RZE_LOG_ERROR("OpenGL 4.5 is needed; the context gives " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << ".");
			AssertFalse();
		}
		RZE_LOG("OpenGL " << reinterpret_cast<const char*>(glGetString(GL_VERSION)) << " on " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

#ifdef _DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(&ReportDebugMessage, nullptr);
#endif

		// D3D's conventions (GLDevice's comment); the back buffer is bound, below
		SetIsBackBufferBound(false);

		int width = 0;
		int height = 0;
		SDL_GetWindowSizeInPixels(m_window, &width, &height);
		CreateBackBuffer(static_cast<U32>(width), static_cast<U32>(height));
	}

	void GLDevice::Shutdown()
	{
		DestroyBackBuffer();
		if (m_context != nullptr)
		{
			SDL_GL_MakeCurrent(m_window, nullptr);
			SDL_GL_DestroyContext(m_context);
			m_context = nullptr;
		}
	}

	void GLDevice::AttachToCurrentThread()
	{
		if (!SDL_GL_MakeCurrent(m_window, m_context))
		{
			RZE_LOG_ERROR("Couldn't make the OpenGL context current: " << SDL_GetError());
			AssertFalse();
		}
	}

	void GLDevice::DetachFromCurrentThread()
	{
		SDL_GL_MakeCurrent(m_window, nullptr);
	}

	std::shared_ptr<IVertexBuffer> GLDevice::CreateVertexBuffer()
	{
		return std::make_shared<GLVertexBuffer>(*this);
	}

	std::shared_ptr<IIndexBuffer> GLDevice::CreateIndexBuffer()
	{
		return std::make_shared<GLIndexBuffer>(*this);
	}

	std::shared_ptr<IConstantBuffer> GLDevice::CreateConstantBuffer(U32 alignment)
	{
		return std::make_shared<GLConstantBuffer>(*this, alignment);
	}

	std::shared_ptr<ITextureBuffer2D> GLDevice::CreateTextureBuffer2D()
	{
		return std::make_shared<GLTextureBuffer2D>(*this);
	}

	std::shared_ptr<IVertexShader> GLDevice::CreateVertexShader()
	{
		return std::make_shared<GLVertexShader>(*this);
	}

	std::shared_ptr<IPixelShader> GLDevice::CreatePixelShader()
	{
		return std::make_shared<GLPixelShader>(*this);
	}

	std::shared_ptr<IComputeShader> GLDevice::CreateComputeShader()
	{
		return std::make_shared<GLComputeShader>(*this);
	}

	void GLDevice::CreateBackBuffer(U32 width, U32 height)
	{
		m_backBufferWidth = width > 0 ? width : 1;
		m_backBufferHeight = height > 0 ? height : 1;

		// RGBA8 colour and D24S8 depth, as the D3D back buffer and its depth-stencil are
		glCreateTextures(GL_TEXTURE_2D, 1, &m_backBufferColour);
		glTextureStorage2D(m_backBufferColour, 1, GL_RGBA8, static_cast<GLsizei>(m_backBufferWidth), static_cast<GLsizei>(m_backBufferHeight));
		glCreateRenderbuffers(1, &m_backBufferDepthStencil);
		glNamedRenderbufferStorage(m_backBufferDepthStencil, GL_DEPTH24_STENCIL8, static_cast<GLsizei>(m_backBufferWidth), static_cast<GLsizei>(m_backBufferHeight));

		glCreateFramebuffers(1, &m_backBufferFramebuffer);
		glNamedFramebufferTexture(m_backBufferFramebuffer, GL_COLOR_ATTACHMENT0, m_backBufferColour, 0);
		glNamedFramebufferRenderbuffer(m_backBufferFramebuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_backBufferDepthStencil);
		AssertExpr(glCheckNamedFramebufferStatus(m_backBufferFramebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

		glBindFramebuffer(GL_FRAMEBUFFER, m_backBufferFramebuffer);
		SetIsBackBufferBound(true);
	}

	void GLDevice::SetIsBackBufferBound(bool isBackBufferBound)
	{
		m_isBackBufferBound = isBackBufferBound;
		glClipControl(isBackBufferBound ? GL_LOWER_LEFT : GL_UPPER_LEFT, GL_ZERO_TO_ONE);
	}

	void GLDevice::DestroyBackBuffer()
	{
		glDeleteFramebuffers(1, &m_backBufferFramebuffer);
		glDeleteRenderbuffers(1, &m_backBufferDepthStencil);
		glDeleteTextures(1, &m_backBufferColour);
		m_backBufferFramebuffer = 0;
		m_backBufferDepthStencil = 0;
		m_backBufferColour = 0;
	}

	void GLDevice::Present()
	{
		if (m_isSyncIntervalChanged)
		{
			// 0 presents at once, as D3D's does with tearing
			SDL_GL_SetSwapInterval(static_cast<int>(m_syncInterval));
			m_isSyncIntervalChanged = false;
		}

		int windowWidth = 0;
		int windowHeight = 0;
		SDL_GetWindowSizeInPixels(m_window, &windowWidth, &windowHeight);

		// As it is: the back buffer is drawn the way up the window shows it (GLDevice's comment). A blit is only free of
		// the scissor test when it's off, and ImGui's renderer leaves it as it found it.
		glDisable(GL_SCISSOR_TEST);
		glBlitNamedFramebuffer(m_backBufferFramebuffer, 0,
			0, 0, static_cast<GLint>(m_backBufferWidth), static_cast<GLint>(m_backBufferHeight),
			0, 0, windowWidth, windowHeight,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		SDL_GL_SwapWindow(m_window);
	}

	void GLDevice::HandleWindowResize(U32 newWidth, U32 newHeight)
	{
		DestroyBackBuffer();
		CreateBackBuffer(newWidth, newHeight);
	}

	void GLDevice::SetSyncInterval(U32 interval)
	{
		m_syncInterval = interval;
		m_isSyncIntervalChanged = true;
	}

	bool GLDevice::WaitForPresentSlot(U32 /*timeoutMS*/)
	{
		return true;
	}

	void GLDevice::BeginDrawSet()
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	bool GLDevice::IsProfilerAttached()
	{
		// Groups are cheap, and there's no asking whether a tool is listening
		return true;
	}

	void GLDevice::BeginProfilerEvent(const char* name)
	{
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
	}

	void GLDevice::EndProfilerEvent()
	{
		glPopDebugGroup();
	}

	void GLDevice::ClearDepthStencilBuffer(ITextureBuffer2D& /*depthTexture*/, bool /*everySlice*/, U32 /*slice*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::ClearRenderTargets()
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::SetRenderTargetBackBuffer()
	{
		// D3D's clears ignore the scissor rectangle, GL's don't
		glDisable(GL_SCISSOR_TEST);
		glClearNamedFramebufferfv(m_backBufferFramebuffer, GL_COLOR, 0, k_backBufferClearColour);
		glClearNamedFramebufferfi(m_backBufferFramebuffer, GL_DEPTH_STENCIL, 0, 1.0f, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, m_backBufferFramebuffer);
		SetIsBackBufferBound(true);
	}

	void GLDevice::SetDepthTarget(ITextureBuffer2D& /*depthTexture*/, U32 /*slice*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
		SetIsBackBufferBound(false);
	}

	void GLDevice::SetColourTarget(ITextureBuffer2D& /*colourTarget*/, ITextureBuffer2D* /*depthTexture*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
		SetIsBackBufferBound(false);
	}

	void GLDevice::GenerateMips(ITextureBuffer2D& /*texture*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::CopyTexture(ITextureBuffer2D& /*source*/, ITextureBuffer2D& /*destination*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::CopyTextureRegion(ITextureBuffer2D& /*source*/, U32 /*sourceX*/, U32 /*sourceY*/, U32 /*width*/, U32 /*height*/, ITextureBuffer2D& /*destination*/, U32 /*destinationX*/, U32 /*destinationY*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::ClearRenderTarget(ITextureBuffer2D& /*renderTarget*/, const Vector4D& /*colour*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::SetViewport(const ViewportParams& params)
	{
		// TopLeftY counts down from the target's top. So does GL's y with the upper-left origin; on the back buffer, drawn
		// GL's way up, it counts up from the bottom.
		const float y = m_isBackBufferBound ? static_cast<float>(m_backBufferHeight) - params.TopLeftY - params.Height : params.TopLeftY;
		glViewportIndexedf(0, params.TopLeftX, y, params.Width, params.Height);
		glDepthRangeIndexed(0, params.MinDepth, params.MaxDepth);
	}

	void GLDevice::SetPrimitiveTopology(EPrimitiveTopology /*topology*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::SetRasterizerState(ERasterizerState /*state*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::SetBlendState(EBlendState /*state*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::SetDepthState(EDepthState /*state*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::UnsetPixelShader()
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::UnsetTextureResource(U32 /*textureSlot*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::UnsetTextureResourceCS(U32 /*textureSlot*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::SetUnorderedAccess(ITextureBuffer2D& /*texture*/, U32 /*slot*/, U32 /*mip*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::UnsetUnorderedAccess(U32 /*slot*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::Draw(U32 /*vertexCount*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::DrawIndexed(U32 /*indexCount*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::Dispatch(U32 /*groupsX*/, U32 /*groupsY*/, U32 /*groupsZ*/)
	{
		RZE_GL_NOT_IMPLEMENTED();
	}

	void GLDevice::ReadTexel(ITextureBuffer2D& /*texture*/, U32 /*x*/, U32 /*y*/, TexelReadback& result)
	{
		RZE_GL_NOT_IMPLEMENTED();
		// Still ready, so nothing waits on it forever
		result.Value = 0;
		result.IsReady.store(true, std::memory_order_release);
	}

	void GLDevice::ReadTexture(ITextureBuffer2D& texture, TextureReadback& result)
	{
		RZE_GL_NOT_IMPLEMENTED();
		// Zeros the size the texture is, and still ready, so nothing waits on it forever
		const GFXTextureBufferParams& params = static_cast<GLTextureBuffer2D&>(texture).GetParams();
		result.Width = params.Width;
		result.Height = params.Height;
		result.BytesPerTexel = GetBytesPerTexel(params);
		result.Data.assign(static_cast<size_t>(result.Width) * result.Height * result.BytesPerTexel, 0);
		result.IsReady.store(true, std::memory_order_release);
	}
}
