#include <StdAfx.h>
#include <Graphics/RenderEngine.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Graphics/MeshGeometry.h>
#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>

#include <Rendering/Renderer.h>

RenderEngine::RenderEngine()
	: m_renderer(std::make_unique<Rendering::Renderer>())
{
	
}

RenderEngine::~RenderEngine()
{
	
}

void RenderEngine::Initialize()
{
	m_renderer->Initialize();
}

void RenderEngine::Update()
{
}

void RenderEngine::Render()
{
	// #TODO This needs to become passing a command list off to the renderer
	m_renderer->Render();
}

void RenderEngine::Shutdown()
{
	m_renderer->ShutDown();
}

void RenderEngine::SetCameraData(const CameraData& cameraData)
{
	// #TODO Eventually this will be a command and RenderEngine wont need to know about Rendering::CameraData
	const Rendering::CameraData* renderingCam;
	// Super lazy hacky way to just "copy" the data over since the structures are identical
	renderingCam = reinterpret_cast<const Rendering::CameraData*>(&cameraData);

	m_renderer->SetCameraData(*renderingCam);
}

RenderProxy* RenderEngine::CreateRenderProxy(const std::vector<MeshGeometry>& meshGeometries, const Matrix4x4& transform)
{
	ResourceHandler& resourceHandler = RZE::GetResourceHandler();
	
	RenderProxy* renderProxy = new RenderProxy();

	for (auto& meshGeometry : meshGeometries)
	{
		const Material& material = meshGeometry.GetMaterial();

		Rendering::MeshData meshData;
		meshData.Vertices = meshGeometry.GetVertexDataRaw();
		meshData.Indices = meshGeometry.GetIndexDataRaw();

		meshData.Material.mProperties.Shininess = material.Shininess;
		meshData.Material.mProperties.Opacity = material.Opacity;

		const ShaderTechnique* const shader = resourceHandler.GetResource<ShaderTechnique>(material.GetShaderResource());
		AssertNotNull(shader);
		meshData.Material.mShaderID = shader->GetHardwareID();

		std::vector<Rendering::TextureData> textureData;
		textureData.reserve(Material::TEXTURE_SLOT_COUNT);
		for (size_t textureSlot = 0; textureSlot < Material::TEXTURE_SLOT_COUNT; ++textureSlot)
		{
			// #TODO We should really be writing code that can deal with permutations of valid textures.
			// Likely via a shader infrastructure that can validate the data needed and pair with its materials etc
			const Texture2D* const texture = resourceHandler.GetResource<Texture2D>(material.GetTexture(textureSlot));
			if (texture != nullptr)
			{
				Rendering::TextureData data;
				// Can solve this problem with a Vector2D(int) maybe?
				data.mHeader.mWidth = static_cast<int>(texture->GetDimensions().X());
				data.mHeader.mHeight = static_cast<int>(texture->GetDimensions().Y());
				data.mData = texture->GetRawData();
				textureData.emplace_back(std::move(data));
			}
		}

		Rendering::RenderObject* renderObject = m_renderer->CreateAndInitializeRenderObject(meshData, textureData, transform);
		renderProxy->m_renderObjects.push_back(renderObject);
	}

	return renderProxy;
}

Int32 RenderEngine::CreatePixelShader(const FilePath& filePath)
{
	return m_renderer->CreatePixelShader(filePath);
}

void RenderEngine::SetWindow(void* windowHandle)
{
	m_renderer->SetWindow(windowHandle);
}

void RenderEngine::ResizeCanvas(const Vector2D& newSize)
{
	m_renderer->ResizeCanvas(newSize);
}
