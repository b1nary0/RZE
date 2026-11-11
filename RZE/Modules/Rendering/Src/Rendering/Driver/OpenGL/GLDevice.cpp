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

#include <algorithm>
#include <cstring>
#include <string>

namespace Rendering
{
	namespace
	{
		// As DX11Device::SetRenderTargetBackBuffer clears to
		constexpr GLfloat k_backBufferClearColour[4] = { 0.25f, 0.45f, 0.65f, 1.0f };

		// The samplers the shaders' s registers mean, as the DX11 backend binds them: s0 is the one every texture binds
		// with itself (linear and wrapping), s1 the shadow maps' comparison, s2 linear and clamped, s3 point and clamped
		enum ESampler : U32
		{
			Sampler_LinearWrap,
			Sampler_ShadowCompare,
			Sampler_LinearClamp,
			Sampler_PointClamp,

			Sampler_Count
		};

		// As DX11Device's: a depth-stencil value of 1 and stencil 0, packed as GL_UNSIGNED_INT_24_8 packs them
		constexpr GLuint k_clearedDepthStencil = 0xFFFFFF00u;

		GLint GetComponentCount(EDataFormat format)
		{
			switch (format)
			{
			case EDataFormat::R32G32_FLOAT: return 2;
			case EDataFormat::R32G32B32_FLOAT: return 3;
			case EDataFormat::R32G32B32A32_FLOAT: return 4;
			}
			return 4;
		}

		// A D24S8 texel as GL reads it (depth in the high 24 bits, stencil in the low 8) in D3D's order (depth in the low
		// 24 bits), which RenderView::DecodeDepth reads
		U32 ToD3DDepthStencil(U32 texel)
		{
			return (texel >> 8) | ((texel & 0xFFu) << 24);
		}

		GLTextureBuffer2D& AsGL(ITextureBuffer2D& texture)
		{
			// Every resource the device is given came from its own Create functions
			return static_cast<GLTextureBuffer2D&>(texture);
		}

		// KHR_debug's reports, which a debug context gives as they happen, on the thread that made the call
		void GLAPIENTRY ReportDebugMessage(GLenum /*source*/, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
		{
			if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
			{
				return;
			}

			switch (type)
			{
			case GL_DEBUG_TYPE_ERROR:
				RZE_LOG_ERROR("OpenGL (" << id << "): " << message);
				AssertMsg(false, "An OpenGL call failed (see the log)");
				break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
				RZE_LOG_WARNING("OpenGL (" << id << "): " << message);
				break;
			default:
				// The driver's notes on what's slow, e.g. a shader recompiled for the state it's drawn with, and portability
				// notes: Mesa checks every draw against OpenGL ES 3.1's rules, which don't apply to desktop GL 4.5
				RZE_LOG_DEBUG("OpenGL (" << id << "): " << message);
				break;
			}
		}
	}

	void GLDevice::SetWindow(void* windowHandle)
	{
		m_window = static_cast<SDL_Window*>(windowHandle);
	}

	void GLDevice::Initialize(bool validate)
	{
		AssertNotNull(m_window);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, validate ? SDL_GL_CONTEXT_DEBUG_FLAG : 0);

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

		if (validate)
		{
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(&ReportDebugMessage, nullptr);
		}

		// D3D's conventions (GLDevice's comment); the back buffer is bound, below
		SetIsBackBufferBound(false);
		// D3D samples across a cube map's face edges
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		// Rows read back as tightly as D3D's staging copies give them, and upload as tightly as D3D takes them (row
		// pitch = width x texel size), whatever their width: GL's default starts each row on 4 bytes, which a 1- or
		// 2-byte format's rows needn't
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glCreateProgramPipelines(1, &m_graphicsPipeline);
		glCreateProgramPipelines(1, &m_computePipeline);
		glCreateVertexArrays(1, &m_vertexArray);

		static_assert(Sampler_Count == k_samplerCount, "A sampler for every s register the shaders use");
		glCreateSamplers(static_cast<GLsizei>(m_samplers.size()), m_samplers.data());
		// As DX11Device's mLinearWrapSamplerState
		glSamplerParameteri(m_samplers[Sampler_LinearWrap], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(m_samplers[Sampler_LinearWrap], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(m_samplers[Sampler_LinearWrap], GL_TEXTURE_WRAP_S, GL_REPEAT);
		glSamplerParameteri(m_samplers[Sampler_LinearWrap], GL_TEXTURE_WRAP_T, GL_REPEAT);
		glSamplerParameteri(m_samplers[Sampler_LinearWrap], GL_TEXTURE_WRAP_R, GL_REPEAT);
		// As DX11Device's mShadowSamplerState: linear comparison, mip point, and outside the map counts as lit
		const GLfloat lit[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glSamplerParameteri(m_samplers[Sampler_ShadowCompare], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glSamplerParameteri(m_samplers[Sampler_ShadowCompare], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(m_samplers[Sampler_ShadowCompare], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(m_samplers[Sampler_ShadowCompare], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glSamplerParameteri(m_samplers[Sampler_ShadowCompare], GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glSamplerParameterfv(m_samplers[Sampler_ShadowCompare], GL_TEXTURE_BORDER_COLOR, lit);
		glSamplerParameteri(m_samplers[Sampler_ShadowCompare], GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glSamplerParameteri(m_samplers[Sampler_ShadowCompare], GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		// As DX11Device's mLinearClampSamplerState
		glSamplerParameteri(m_samplers[Sampler_LinearClamp], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(m_samplers[Sampler_LinearClamp], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(m_samplers[Sampler_LinearClamp], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(m_samplers[Sampler_LinearClamp], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(m_samplers[Sampler_LinearClamp], GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		// As DX11Device's mPointClampSamplerState
		glSamplerParameteri(m_samplers[Sampler_PointClamp], GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glSamplerParameteri(m_samplers[Sampler_PointClamp], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(m_samplers[Sampler_PointClamp], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(m_samplers[Sampler_PointClamp], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(m_samplers[Sampler_PointClamp], GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		// What D3D draws with before anything's set
		SetRasterizerState(ERasterizerState::Default);
		SetBlendState(EBlendState::Opaque);
		SetDepthState(EDepthState::ReadWrite);
		m_primitiveMode = GL_TRIANGLES;

		int width = 0;
		int height = 0;
		SDL_GetWindowSizeInPixels(m_window, &width, &height);
		CreateBackBuffer(static_cast<U32>(width), static_cast<U32>(height));
	}

	void GLDevice::Shutdown()
	{
		DeleteReleasedObjects();
		for (const std::pair<const FramebufferKey, U32>& framebuffer : m_framebuffers)
		{
			glDeleteFramebuffers(1, &framebuffer.second);
		}
		m_framebuffers.clear();
		glDeleteSamplers(static_cast<GLsizei>(m_samplers.size()), m_samplers.data());
		glDeleteVertexArrays(1, &m_vertexArray);
		glDeleteProgramPipelines(1, &m_graphicsPipeline);
		glDeleteProgramPipelines(1, &m_computePipeline);
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

	void GLDevice::DeleteLater(EObjectType type, U32 name)
	{
		std::lock_guard lock(m_releasedObjectsMutex);
		m_releasedObjects.emplace_back(type, name);
	}

	void GLDevice::DeleteReleasedObjects()
	{
		std::vector<std::pair<EObjectType, U32>> released;
		{
			std::lock_guard lock(m_releasedObjectsMutex);
			released.swap(m_releasedObjects);
		}

		for (const std::pair<EObjectType, U32>& object : released)
		{
			const GLuint name = object.second;
			switch (object.first)
			{
			case EObjectType::Program: glDeleteProgram(name); break;
			case EObjectType::Buffer: glDeleteBuffers(1, &name); break;
			case EObjectType::Renderbuffer: glDeleteRenderbuffers(1, &name); break;
			case EObjectType::Framebuffer: glDeleteFramebuffers(1, &name); break;
			case EObjectType::Sampler: glDeleteSamplers(1, &name); break;
			case EObjectType::VertexArray: glDeleteVertexArrays(1, &name); break;
			case EObjectType::Texture:
			{
				// And everything still holding it: the framebuffers it's attached to, and the slots it's bound to, as D3D
				// releases a texture's views and bindings with it
				for (auto framebuffer = m_framebuffers.begin(); framebuffer != m_framebuffers.end();)
				{
					if (std::get<0>(framebuffer->first) == name || std::get<1>(framebuffer->first) == name)
					{
						glDeleteFramebuffers(1, &framebuffer->second);
						framebuffer = m_framebuffers.erase(framebuffer);
					}
					else
					{
						++framebuffer;
					}
				}
				std::replace(m_pixelTextures.begin(), m_pixelTextures.end(), name, 0u);
				std::replace(m_computeTextures.begin(), m_computeTextures.end(), name, 0u);
				glDeleteTextures(1, &name);
			}
			break;
			}
		}
	}

	void GLDevice::SetVertexShader(const GLVertexShader& shader)
	{
		m_vertexShader = &shader;
		glUseProgramStages(m_graphicsPipeline, GL_VERTEX_SHADER_BIT, shader.GetProgram().GetProgram());
	}

	void GLDevice::SetPixelShader(const GLPixelShader& shader)
	{
		m_pixelShader = &shader;
		glUseProgramStages(m_graphicsPipeline, GL_FRAGMENT_SHADER_BIT, shader.GetProgram().GetProgram());
	}

	void GLDevice::SetComputeShader(const GLComputeShader& shader)
	{
		m_computeShader = &shader;
		glUseProgramStages(m_computePipeline, GL_COMPUTE_SHADER_BIT, shader.GetProgram().GetProgram());
	}

	void GLDevice::SetInputLayout(const GLVertexShader& shader)
	{
		for (U32 location = 0; location < 32; ++location)
		{
			if (m_enabledAttributeMask & (1u << location))
			{
				glDisableVertexArrayAttrib(m_vertexArray, location);
			}
		}
		m_enabledAttributeMask = 0;

		// From vertex buffer slot 0, the one D3D's input layouts here read from
		for (const GLVertexShader::Attribute& attribute : shader.GetAttributes())
		{
			glEnableVertexArrayAttrib(m_vertexArray, attribute.Location);
			glVertexArrayAttribFormat(m_vertexArray, attribute.Location, GetComponentCount(attribute.Format), GL_FLOAT, GL_FALSE, attribute.Offset);
			glVertexArrayAttribBinding(m_vertexArray, attribute.Location, 0);
			m_enabledAttributeMask |= 1u << attribute.Location;
			if (attribute.IsPerInstance)
			{
				glVertexArrayBindingDivisor(m_vertexArray, 0, 1);
			}
		}
	}

	void GLDevice::SetVertexBuffer(const GLVertexBuffer& buffer, U32 slot)
	{
		glVertexArrayVertexBuffer(m_vertexArray, slot, buffer.GetBuffer(), 0, static_cast<GLsizei>(buffer.GetStride()));
	}

	void GLDevice::SetIndexBuffer(U32 buffer)
	{
		glVertexArrayElementBuffer(m_vertexArray, buffer);
	}

	void GLDevice::SetTextureResource(GLShaderTranslator::EStage stage, U32 slot, const GLTextureBuffer2D* texture)
	{
		AssertExpr(slot < k_textureSlotCount);
		std::array<U32, k_textureSlotCount>& slots = stage == GLShaderTranslator::EStage::Compute ? m_computeTextures : m_pixelTextures;
		slots[slot] = texture != nullptr ? texture->GetTexture() : 0;
	}

	void GLDevice::BindTextures(const GLShaderTranslator::Result& program, const std::array<U32, k_textureSlotCount>& slots)
	{
		for (const GLShaderTranslator::CombinedSampler& combined : program.CombinedSamplers)
		{
			glBindTextureUnit(combined.Unit, slots[combined.TextureSlot]);
			// A texture only read with Load has no sampler, so its own parameters apply (GLTextureBuffer2D::Allocate)
			glBindSampler(combined.Unit, combined.SamplerSlot >= 0 ? m_samplers[combined.SamplerSlot] : 0);
		}
	}

	void GLDevice::CheckVaryings()
	{
		if (m_vertexShader == nullptr || m_pixelShader == nullptr)
		{
			return;
		}

		const std::pair<U32, U32> pair(m_vertexShader->GetProgram().GetProgram(), m_pixelShader->GetProgram().GetProgram());
		if (!m_checkedVaryings.insert(pair).second)
		{
			return;
		}

		const std::vector<GLShaderTranslator::Varying>& outputs = m_vertexShader->GetProgram().GetTranslation().Outputs;
		for (const GLShaderTranslator::Varying& input : m_pixelShader->GetProgram().GetTranslation().Inputs)
		{
			const bool isWritten = std::any_of(outputs.begin(), outputs.end(), [&input](const GLShaderTranslator::Varying& output)
				{
					return output.Semantic == input.Semantic && output.Location == input.Location;
				});
			if (!isWritten)
			{
				RZE_LOG_ERROR("OpenGL: a pixel shader reads " << input.Semantic << " at location " << input.Location << ", where the vertex shader drawn with it doesn't write it.");
			}
		}
	}

	void GLDevice::PrepareDraw()
	{
		// ImGui's renderer uses programs directly, which would take the pipeline's place
		glUseProgram(0);
		glBindProgramPipeline(m_graphicsPipeline);
		glBindVertexArray(m_vertexArray);
		if (m_pixelShader != nullptr)
		{
			BindTextures(m_pixelShader->GetProgram().GetTranslation(), m_pixelTextures);
		}
		CheckVaryings();
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

	U32 GLDevice::GetFramebuffer(const FramebufferKey& key)
	{
		const auto known = m_framebuffers.find(key);
		if (known != m_framebuffers.end())
		{
			return known->second;
		}

		const GLuint colour = std::get<0>(key);
		const GLuint depth = std::get<1>(key);
		const int depthSlice = std::get<2>(key);

		GLuint framebuffer = 0;
		glCreateFramebuffers(1, &framebuffer);
		if (colour != 0)
		{
			// Every slice of an array, as a D3D render target view of one has
			glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, colour, 0);
			glNamedFramebufferDrawBuffer(framebuffer, GL_COLOR_ATTACHMENT0);
		}
		else
		{
			glNamedFramebufferDrawBuffer(framebuffer, GL_NONE);
			glNamedFramebufferReadBuffer(framebuffer, GL_NONE);
		}
		if (depth != 0)
		{
			if (depthSlice >= 0)
			{
				glNamedFramebufferTextureLayer(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, depth, 0, depthSlice);
			}
			else
			{
				glNamedFramebufferTexture(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, depth, 0);
			}
		}

		const GLenum status = glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			RZE_LOG_ERROR("OpenGL: a render target can't be drawn to (framebuffer status 0x" << std::hex << status << std::dec << ").");
		}

		m_framebuffers.emplace(key, framebuffer);
		return framebuffer;
	}

	void GLDevice::BindFramebuffer(const FramebufferKey& key)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, GetFramebuffer(key));
		SetIsBackBufferBound(false);
	}

	void GLDevice::Present()
	{
		// What the frame let go of, which nothing draws with any more
		DeleteReleasedObjects();

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
		// As DX11Device's: the default rasterizer state. The samplers are bound with each draw's textures (BindTextures).
		SetRasterizerState(ERasterizerState::Default);
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

	void GLDevice::ClearDepthStencilBuffer(ITextureBuffer2D& depthTexture, bool everySlice, U32 slice)
	{
		const GLTextureBuffer2D& texture = AsGL(depthTexture);
		if (everySlice)
		{
			glClearTexImage(texture.GetTexture(), 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, &k_clearedDepthStencil);
		}
		else
		{
			const GFXTextureBufferParams& params = texture.GetParams();
			glClearTexSubImage(texture.GetTexture(), 0, 0, 0, static_cast<GLint>(slice), static_cast<GLsizei>(params.Width), static_cast<GLsizei>(params.Height), 1,
				GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, &k_clearedDepthStencil);
		}
	}

	void GLDevice::ClearRenderTargets()
	{
		// Nothing bound, as D3D's OMSetRenderTargets with none
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		SetIsBackBufferBound(false);
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

	void GLDevice::SetDepthTarget(ITextureBuffer2D& depthTexture, U32 slice)
	{
		// One slice of an array (e.g. a cube's face); a single texture's one slice is the whole texture
		const GLTextureBuffer2D& depth = AsGL(depthTexture);
		BindFramebuffer(FramebufferKey(0, depth.GetTexture(), depth.GetLayerCount() > 1 ? static_cast<int>(slice) : -1));
	}

	void GLDevice::SetColourTarget(ITextureBuffer2D& colourTarget, ITextureBuffer2D* depthTexture)
	{
		BindFramebuffer(FramebufferKey(AsGL(colourTarget).GetTexture(), depthTexture != nullptr ? AsGL(*depthTexture).GetTexture() : 0, -1));
	}

	void GLDevice::GenerateMips(ITextureBuffer2D& texture)
	{
		glGenerateTextureMipmap(AsGL(texture).GetTexture());
	}

	void GLDevice::CopyTexture(ITextureBuffer2D& source, ITextureBuffer2D& destination)
	{
		// Every mip of every slice, as D3D's CopyResource
		const GLTextureBuffer2D& from = AsGL(source);
		const GLTextureBuffer2D& to = AsGL(destination);
		const GFXTextureBufferParams& params = from.GetParams();
		const U32 mipCount = std::min(from.GetMipCount(), to.GetMipCount());
		for (U32 mip = 0; mip < mipCount; ++mip)
		{
			const GLsizei width = static_cast<GLsizei>(std::max(params.Width >> mip, 1u));
			const GLsizei height = static_cast<GLsizei>(std::max(params.Height >> mip, 1u));
			glCopyImageSubData(from.GetTexture(), from.GetTarget(), static_cast<GLint>(mip), 0, 0, 0,
				to.GetTexture(), to.GetTarget(), static_cast<GLint>(mip), 0, 0, 0, width, height, static_cast<GLsizei>(from.GetLayerCount()));
		}
	}

	void GLDevice::CopyTextureRegion(ITextureBuffer2D& source, U32 sourceX, U32 sourceY, U32 width, U32 height, ITextureBuffer2D& destination, U32 destinationX, U32 destinationY)
	{
		// The first mip of the first slice, as DX11Device's
		const GLTextureBuffer2D& from = AsGL(source);
		const GLTextureBuffer2D& to = AsGL(destination);
		glCopyImageSubData(from.GetTexture(), from.GetTarget(), 0, static_cast<GLint>(sourceX), static_cast<GLint>(sourceY), 0,
			to.GetTexture(), to.GetTarget(), 0, static_cast<GLint>(destinationX), static_cast<GLint>(destinationY), 0,
			static_cast<GLsizei>(width), static_cast<GLsizei>(height), 1);
	}

	void GLDevice::ClearRenderTarget(ITextureBuffer2D& renderTarget, const Vector4D& colour)
	{
		// The first mip of every slice, as a D3D render target view
		const GLTextureBuffer2D& texture = AsGL(renderTarget);
		if (texture.GetFormat().IsInteger)
		{
			// D3D clears an integer target to the colour's values as integers
			const GLuint values[4] = { static_cast<GLuint>(colour.X()), static_cast<GLuint>(colour.Y()), static_cast<GLuint>(colour.Z()), static_cast<GLuint>(colour.W()) };
			glClearTexImage(texture.GetTexture(), 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, values);
		}
		else
		{
			const GLfloat values[4] = { colour.X(), colour.Y(), colour.Z(), colour.W() };
			glClearTexImage(texture.GetTexture(), 0, GL_RGBA, GL_FLOAT, values);
		}
	}

	void GLDevice::SetViewport(const ViewportParams& params)
	{
		// TopLeftY counts down from the target's top. So does GL's y with the upper-left origin; on the back buffer, drawn
		// GL's way up, it counts up from the bottom.
		const float y = m_isBackBufferBound ? static_cast<float>(m_backBufferHeight) - params.TopLeftY - params.Height : params.TopLeftY;
		glViewportIndexedf(0, params.TopLeftX, y, params.Width, params.Height);
		glDepthRangeIndexed(0, params.MinDepth, params.MaxDepth);
	}

	void GLDevice::SetPrimitiveTopology(EPrimitiveTopology topology)
	{
		switch (topology)
		{
		case EPrimitiveTopology::TriangleStrip: m_primitiveMode = GL_TRIANGLE_STRIP; break;
		case EPrimitiveTopology::LineList: m_primitiveMode = GL_LINES; break;
		case EPrimitiveTopology::TriangleList:
		default: m_primitiveMode = GL_TRIANGLES; break;
		}
	}

	void GLDevice::SetRasterizerState(ERasterizerState state)
	{
		// As DX11Device's rasterizer states: counter-clockwise front faces (GLDevice's comment), and depth clipped
		// except for shadow casters, whose bias pushes them away from the light
		glFrontFace(GL_CCW);
		switch (state)
		{
		case ERasterizerState::ShadowCaster:
			glDisable(GL_CULL_FACE);
			glEnable(GL_DEPTH_CLAMP);
			// D3D's DepthBias 1000 and SlopeScaledDepthBias 2.0, in the same units for a 24-bit depth buffer
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(2.0f, 1000.0f);
			break;
		case ERasterizerState::NoCull:
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_CLAMP);
			glDisable(GL_POLYGON_OFFSET_FILL);
			break;
		case ERasterizerState::Default:
		default:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glDisable(GL_DEPTH_CLAMP);
			glDisable(GL_POLYGON_OFFSET_FILL);
			break;
		}
	}

	void GLDevice::SetBlendState(EBlendState state)
	{
		// As DX11Device's blend states; each leaves the target's alpha as it was
		if (state == EBlendState::Opaque)
		{
			glDisable(GL_BLEND);
			return;
		}

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		switch (state)
		{
		case EBlendState::Premultiplied: glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE); break;
		case EBlendState::Additive: glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE); break;
		case EBlendState::Multiply: glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE); break;
		case EBlendState::AlphaBlend:
		default: glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE); break;
		}
	}

	void GLDevice::SetDepthState(EDepthState state)
	{
		// As DX11Device's depth states
		switch (state)
		{
		case EDepthState::ReadOnly:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glDepthMask(GL_FALSE);
			break;
		case EDepthState::ReadOnlyLessEqual:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_FALSE);
			break;
		case EDepthState::Off:
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			break;
		case EDepthState::ReadWrite:
		default:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glDepthMask(GL_TRUE);
			break;
		}
	}

	void GLDevice::UnsetPixelShader()
	{
		m_pixelShader = nullptr;
		glUseProgramStages(m_graphicsPipeline, GL_FRAGMENT_SHADER_BIT, 0);
	}

	void GLDevice::UnsetTextureResource(U32 textureSlot)
	{
		SetTextureResource(GLShaderTranslator::EStage::Pixel, textureSlot, nullptr);
	}

	void GLDevice::UnsetTextureResourceCS(U32 textureSlot)
	{
		SetTextureResource(GLShaderTranslator::EStage::Compute, textureSlot, nullptr);
	}

	void GLDevice::SetUnorderedAccess(ITextureBuffer2D& texture, U32 slot, U32 mip)
	{
		// Every slice of one mip, written as an array if there's more than one (GLTextureBuffer2D::GetImageTexture). In
		// its own format: the shaders' images have none, so stores convert to it (GLShaderTranslator).
		GLTextureBuffer2D& image = AsGL(texture);
		const GLboolean isArray = image.GetLayerCount() > 1 ? GL_TRUE : GL_FALSE;
		glBindImageTexture(slot, image.GetImageTexture(), static_cast<GLint>(mip), isArray, 0, GL_WRITE_ONLY, image.GetFormat().InternalFormat);
	}

	void GLDevice::UnsetUnorderedAccess(U32 slot)
	{
		glBindImageTexture(slot, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
	}

	void GLDevice::Draw(U32 vertexCount)
	{
		PrepareDraw();
		glDrawArrays(m_primitiveMode, 0, static_cast<GLsizei>(vertexCount));
	}

	void GLDevice::DrawIndexed(U32 indexCount)
	{
		PrepareDraw();
		glDrawElements(m_primitiveMode, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
	}

	void GLDevice::Dispatch(U32 groupsX, U32 groupsY, U32 groupsZ)
	{
		glUseProgram(0);
		glBindProgramPipeline(m_computePipeline);
		if (m_computeShader != nullptr)
		{
			BindTextures(m_computeShader->GetProgram().GetTranslation(), m_computeTextures);
		}
		glDispatchCompute(groupsX, groupsY, groupsZ);
		// D3D makes a dispatch's writes visible to whatever reads them next; GL waits to be told. Every kind of read, since
		// what comes next could sample, draw into or copy what was written.
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void GLDevice::ReadTexel(ITextureBuffer2D& texture, U32 x, U32 y, TexelReadback& result)
	{
		// From the first mip of the first slice, as DX11Device's; GL waits for the GPU to finish drawing it
		const GLTextureBuffer2D& gl = AsGL(texture);
		const GLTextureBuffer2D::Format& format = gl.GetFormat();
		AssertExpr(format.BytesPerTexel <= sizeof(result.Value));

		U64 texel = 0;
		glGetTextureSubImage(gl.GetTexture(), 0, static_cast<GLint>(x), static_cast<GLint>(y), 0, 1, 1, 1,
			format.PixelFormat, format.PixelType, static_cast<GLsizei>(sizeof(texel)), &texel);
		if (format.IsDepth)
		{
			texel = ToD3DDepthStencil(static_cast<U32>(texel));
		}

		result.Value = texel;
		result.IsReady.store(true, std::memory_order_release);
	}

	void GLDevice::ReadTexture(ITextureBuffer2D& texture, TextureReadback& result)
	{
		// The first mip of the first slice, rows top first, as DX11Device's
		const GLTextureBuffer2D& gl = AsGL(texture);
		const GLTextureBuffer2D::Format& format = gl.GetFormat();
		result.Width = gl.GetParams().Width;
		result.Height = gl.GetParams().Height;
		result.BytesPerTexel = format.BytesPerTexel;
		result.Data.resize(static_cast<size_t>(result.Width) * result.Height * result.BytesPerTexel);

		glGetTextureSubImage(gl.GetTexture(), 0, 0, 0, 0, static_cast<GLsizei>(result.Width), static_cast<GLsizei>(result.Height), 1,
			format.PixelFormat, format.PixelType, static_cast<GLsizei>(result.Data.size()), result.Data.data());
		if (format.IsDepth)
		{
			for (size_t offset = 0; offset < result.Data.size(); offset += sizeof(U32))
			{
				U32 texel = 0;
				memcpy(&texel, &result.Data[offset], sizeof(texel));
				texel = ToD3DDepthStencil(texel);
				memcpy(&result.Data[offset], &texel, sizeof(texel));
			}
		}

		result.IsReady.store(true, std::memory_order_release);
	}

	void GLDevice::ReadBackBuffer(TextureReadback& result)
	{
		result.Width = m_backBufferWidth;
		result.Height = m_backBufferHeight;
		result.BytesPerTexel = 4;
		const size_t rowSize = static_cast<size_t>(result.Width) * result.BytesPerTexel;
		std::vector<U8> rows(rowSize * result.Height);
		glGetTextureImage(m_backBufferColour, 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(rows.size()), rows.data());

		// The back buffer is drawn GL's way up (its row 0 is the bottom, as the window shows it), so its rows go in
		// the other order
		result.Data.resize(rows.size());
		for (U32 row = 0; row < result.Height; ++row)
		{
			memcpy(&result.Data[static_cast<size_t>(row) * rowSize], &rows[static_cast<size_t>(result.Height - 1 - row) * rowSize], rowSize);
		}

		result.IsReady.store(true, std::memory_order_release);
	}
}
