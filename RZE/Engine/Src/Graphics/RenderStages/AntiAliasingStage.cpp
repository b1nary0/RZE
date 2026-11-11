#include <Graphics/RenderStages/AntiAliasingStage.h>

#include <Graphics/FullScreenPass.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/TexelBuffer.h>
#include <Rendering/Driver/GFXBuffer.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Vector4D.h>

#include <SMAA/AreaTex.h>
#include <SMAA/SearchTex.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

static_assert(sizeof(areaTexBytes) == AREATEX_SIZE && AREATEX_SIZE == 160 * 560 * 2, "SMAA's area texture is RG8, 160x560");
static_assert(sizeof(searchTexBytes) == SEARCHTEX_SIZE && SEARCHTEX_SIZE == 64 * 16, "SMAA's search texture is R8, 64x16");

namespace
{
	// SMAA's intermediate textures for one view, the size of its target
	struct SMAAState : RenderStageData<SMAAState>
	{
		[[maybe_unused]] static constexpr const char* k_name = "SMAAState";

		// Where each pixel has an edge to its left (r) and top (g) neighbour
		Rendering::RenderTargetHandle Edges;
		// How much each pixel blends with each neighbour across an edge
		Rendering::RenderTargetHandle BlendWeights;
		U32 Width = 0;
		U32 Height = 0;
	};

	Rendering::GFXTextureBufferParams MakeTextureParams(U32 width, U32 height, Rendering::ETextureFormat format)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.Width = width;
		params.Height = height;
		params.MipLevels = 1;
		params.SampleCount = 1;
		params.Format = format;
		return params;
	}

	Rendering::RenderTargetHandle CreateTarget(U32 width, U32 height, Rendering::ETextureFormat format)
	{
		Rendering::GFXTextureBufferParams params = MakeTextureParams(width, height, format);
		params.bIsRenderTarget = true;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	// A copy of one of SMAA's lookup arrays, which the upload frees once the render thread has made the texture
	Rendering::TextureBuffer2DHandle CreateLookupTexture(const unsigned char* bytes, size_t size, U32 width, U32 height, Rendering::ETextureFormat format)
	{
		U8* texels = static_cast<U8*>(std::malloc(size));
		std::memcpy(texels, bytes, size);
		return Rendering::Renderer::CreateTextureBuffer2D(Rendering::TexelBuffer(texels, &std::free), MakeTextureParams(width, height, format));
	}
}

void AntiAliasingStage::Initialize()
{
	Rendering::ShaderInputLayout inputLayout = {};
	m_vertexShader = LoadVertexShader("Vertex_RenderTargetQuad", inputLayout);

	m_fxaaShader = LoadPixelShader("Pixel_FXAA");
	m_fxaaParamsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(FXAAParamsLayout), 16, 1);

	m_smaaEdgeShader = LoadPixelShader("Pixel_SMAAEdgeDetection");
	m_smaaWeightsShader = LoadPixelShader("Pixel_SMAABlendingWeights");
	m_smaaWeightsNoDiagonalsShader = LoadPixelShader("Pixel_SMAABlendingWeightsNoDiagonals");
	m_smaaBlendShader = LoadPixelShader("Pixel_SMAANeighborhoodBlending");
	m_smaaParamsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(SMAAParamsLayout), 16, 1);

	m_areaTexture = CreateLookupTexture(areaTexBytes, sizeof(areaTexBytes), AREATEX_WIDTH, AREATEX_HEIGHT, Rendering::ETextureFormat::RG8_UNORM);
	m_searchTexture = CreateLookupTexture(searchTexBytes, sizeof(searchTexBytes), SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, Rendering::ETextureFormat::R8_UNORM);
}

void AntiAliasingStage::Setup(RenderStageBuilder& builder)
{
	m_tonemappedColourInput = builder.Reads<TonemappedColourData>();
	m_displayColourOutput = builder.Writes<DisplayColourData>();
}

void AntiAliasingStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	const TonemappedColourData& tonemapped = m_tonemappedColourInput.Get(context);

	DisplayColourData displayColour;
	displayColour.Colour = view.Target->GetTargetPlatformObject();
	displayColour.Depth = tonemapped.Depth;

	if (view.AntiAliasing.Method != EAntiAliasingMethod::Off)
	{
		Rendering::Renderer::Begin("AntiAliasingStage");

		Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleStrip);
		Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
		Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y() });

		if (view.AntiAliasing.Method == EAntiAliasingMethod::SMAA)
		{
			DrawSMAA(context, tonemapped.Colour, displayColour.Colour);
		}
		else
		{
			DrawFXAA(view, tonemapped.Colour, displayColour.Colour);
		}

		Rendering::Renderer::End();
	}

	m_displayColourOutput.Publish(context, displayColour);
}

void AntiAliasingStage::DrawFXAA(const RenderView& view, const Rendering::TextureBuffer2DHandle& source, const Rendering::RenderTargetHandle& target)
{
	const Rendering::RenderTargetTexture& renderTarget = *view.Target;

	const FXAASettings settings = GetFXAASettings(view.AntiAliasing.Quality);
	FXAAParamsLayout params = {};
	params.ViewportScale[0] = view.ViewportSize.X() / static_cast<float>(renderTarget.GetWidth());
	params.ViewportScale[1] = view.ViewportSize.Y() / static_cast<float>(renderTarget.GetHeight());
	params.EdgeThreshold = settings.EdgeThreshold;
	params.EdgeThresholdMin = settings.EdgeThresholdMin;
	params.Subpixel = settings.Subpixel;
	params.SearchStepCount = static_cast<float>(settings.SearchStepCount);
	std::copy(settings.SearchSteps.begin(), settings.SearchSteps.end(), params.SearchSteps);

	Rendering::Renderer::UploadDataToBuffer<FXAAParamsLayout>(m_fxaaParamsBuffer, &params);
	Rendering::Renderer::SetConstantBufferPS(m_fxaaParamsBuffer, 0);

	Rendering::Renderer::SetColourTarget(target);
	DrawFullScreenPass(m_fxaaShader, { source });
}

void AntiAliasingStage::DrawSMAA(RenderContext& context, const Rendering::TextureBuffer2DHandle& source, const Rendering::RenderTargetHandle& target)
{
	const RenderView& view = context.View;
	const Rendering::RenderTargetTexture& renderTarget = *view.Target;

	SMAAState& state = context.Persistent().GetOrCreate<SMAAState>([]() { return SMAAState(); });
	if (state.Width != renderTarget.GetWidth() || state.Height != renderTarget.GetHeight())
	{
		state.Width = renderTarget.GetWidth();
		state.Height = renderTarget.GetHeight();
		state.Edges = CreateTarget(state.Width, state.Height, Rendering::ETextureFormat::RG8_UNORM);
		state.BlendWeights = CreateTarget(state.Width, state.Height, Rendering::ETextureFormat::RGBA8_UNORM);
	}

	const SMAASettings settings = GetSMAASettings(view.AntiAliasing.Quality);
	const float width = static_cast<float>(state.Width);
	const float height = static_cast<float>(state.Height);
	SMAAParamsLayout params = {};
	params.RTMetrics[0] = 1.0f / width;
	params.RTMetrics[1] = 1.0f / height;
	params.RTMetrics[2] = width;
	params.RTMetrics[3] = height;
	params.Threshold = settings.Threshold;
	params.MaxSearchSteps = static_cast<float>(settings.SearchSteps);
	params.MaxSearchStepsDiag = static_cast<float>(settings.DiagonalSearchSteps);
	params.CornerRounding = settings.CornerRounding;

	Rendering::Renderer::UploadDataToBuffer<SMAAParamsLayout>(m_smaaParamsBuffer, &params);
	Rendering::Renderer::SetConstantBufferPS(m_smaaParamsBuffer, 0);

	// Edge detection discards pixels without an edge, and the passes after it read a pixel past the viewport's right
	// and bottom edges: cleared, those read as no edge and no blend, whatever an earlier frame left there
	const Vector4D none(0.0f, 0.0f, 0.0f, 0.0f);
	Rendering::Renderer::ClearRenderTarget(state.Edges, none);
	Rendering::Renderer::ClearRenderTarget(state.BlendWeights, none);

	Rendering::Renderer::SetColourTarget(state.Edges);
	DrawFullScreenPass(m_smaaEdgeShader, { source });

	Rendering::Renderer::SetColourTarget(state.BlendWeights);
	DrawFullScreenPass(settings.DetectsDiagonalsAndCorners ? m_smaaWeightsShader : m_smaaWeightsNoDiagonalsShader,
		{ state.Edges, m_areaTexture, m_searchTexture });

	Rendering::Renderer::SetColourTarget(target);
	DrawFullScreenPass(m_smaaBlendShader, { source, state.BlendWeights });
}
