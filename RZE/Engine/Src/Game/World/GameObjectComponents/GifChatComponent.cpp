#include <Game/World/GameObjectComponents/GifChatComponent.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/MeshComponentProperties.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/RenderScene.h>
#include <Graphics/Texture2D.h>

#include <Utils/Memory/ByteStream.h>

#include <STB/stb_image.h>

#include <cstring>
#include <limits>
#include <memory>
#include <string>

namespace
{
	enum GifFilepathIndex
	{
		JOEY_WINK = 0,
		PULP_FICTION_TRAVOLTA
	};

	// Plain strings: a Filepath built at static init would resolve before main sets the directory context
	const char* const gifPaths[] =
	{
		"Assets/2D/GIFs/friends-joey-tribbiani.gif",
		"Assets/2D/GIFs/pulp_fiction_travolta.gif"
	};

	// Frees a frame's texels, which are allocated with new[]
	void FreeFrameTexels(void* texels)
	{
		delete[] static_cast<U8*>(texels);
	}
}

GifChatComponent::GifChatComponent()
{
}

void GifChatComponent::Initialize()
{
	m_renderScene = &GetOwner()->GetScene().GetRenderScene();

	Load(Filepath(gifPaths[GifFilepathIndex::PULP_FICTION_TRAVOLTA]));
}

void GifChatComponent::Uninitialize()
{
	// The mesh holds the material, and the material a frame
	m_mesh.reset();
	m_material.reset();
	m_frames.clear();
	m_frameDelays.clear();
	m_totalFrames = 0;
	m_currentDisplayingFrame = 0;
	m_elapsedMS = 0.0f;
}

void GifChatComponent::OnAddToScene()
{
	CreateRenderObject();
}

void GifChatComponent::OnRemoveFromScene()
{
	if (m_meshRenderObject != nullptr)
	{
		m_renderScene->DestroyRenderObject(m_meshRenderObject);
	}
}

void GifChatComponent::Update()
{
	m_elapsedMS += RZE().GetDeltaTimeMS();

	if (m_currentDisplayingFrame < m_totalFrames)
	{
		if (m_elapsedMS >= static_cast<float>(m_frameDelays[m_currentDisplayingFrame]))
		{
			m_material->SetTexture(0, m_frames[m_currentDisplayingFrame]);
			++m_currentDisplayingFrame;

			m_elapsedMS = 0;
		}
	}
	else
	{
		m_currentDisplayingFrame = 0;
	}

	if (m_meshRenderObject != nullptr)
	{
		GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();
		AssertMsg(transformComponent != nullptr, "No TransformComponent found. Mesh creation is useless without a location in 3D space.");

		// @TODO Look into ways to avoid this
		m_meshRenderObject->SetTransform(transformComponent->GetWorldMatrix());
	}
}

void GifChatComponent::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
{
	// @TODO Look at moving this out so that GameObject::Serialize actually
	// writes the name of the component?
	writer.String("GifChatComponent");
	writer.StartObject();
	{
	}
	writer.EndObject();
}

void GifChatComponent::Deserialize(const rapidjson::Value& data)
{
}

void GifChatComponent::Load(const Filepath& fp)
{
	ByteStream fileBytes(fp.GetRelativePath());
	if (!fileBytes.ReadFromFile(fp) || fileBytes.GetLength() > static_cast<size_t>(std::numeric_limits<int>::max()))
	{
		RZE_LOG_ERROR_ARGS("GifChat couldn't read [%s].", fp.GetRelativePath().c_str());
		return;
	}

	int width = 0;
	int height = 0;
	int frameCount = 0;
	int channelsInFile = 0;
	int* delays = nullptr;
	// Every frame in turn, as RGBA
	stbi_uc* const frameData = stbi_load_gif_from_memory(fileBytes.GetBytes(), static_cast<int>(fileBytes.GetLength()), &delays,
		&width, &height, &frameCount, &channelsInFile, STBI_rgb_alpha);
	if (frameData == nullptr || frameCount <= 0)
	{
		RZE_LOG_ERROR_ARGS("GifChat couldn't decode [%s]: %s.", fp.GetRelativePath().c_str(), stbi_failure_reason());
		stbi_image_free(frameData);
		stbi_image_free(delays);
		return;
	}

	if (delays != nullptr)
	{
		m_frameDelays.assign(delays, delays + frameCount);
	}
	else
	{
		m_frameDelays.assign(frameCount, 0);
	}
	stbi_image_free(delays);

	// Unnamed resources: made here rather than loaded from a file, so nothing can look them up, and each GifChat gets
	// its own. The name is for the Resource Monitor.
	const std::string gifName = fp.GetFileName();
	const size_t frameSizeBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * STBI_rgb_alpha;
	m_frames.reserve(frameCount);
	for (int frame = 0; frame < frameCount; ++frame)
	{
		std::unique_ptr<Texture2D> frameTexture = std::make_unique<Texture2D>();

		// Each frame's texture frees its texels once they're uploaded, so it gets a copy of its slice of the GIF
		U8* const frameTexels = new U8[frameSizeBytes];
		std::memcpy(frameTexels, frameData + frame * frameSizeBytes, frameSizeBytes);
		frameTexture->Load(Rendering::TexelBuffer(frameTexels, &FreeFrameTexels), width, height);

		m_frames.push_back(RZE().GetResourceHandler().AddUnnamedResource(std::move(frameTexture), gifName + ", frame " + std::to_string(frame)));
	}

	// Frames own copies of their data now
	stbi_image_free(frameData);
	m_totalFrames = frameCount;

	m_material = std::make_shared<MaterialInstance>("GifMaterial");
	m_material->SetTexture(0, m_frames[m_currentDisplayingFrame]);
	m_material->SetPixelShader(LoadPixelShader("Pixel_NewRenderer_DiffuseOnly"));

	GenerateMesh(m_material);
}

void GifChatComponent::CreateRenderObject()
{
	// The GIF didn't load
	if (m_mesh == nullptr)
	{
		return;
	}

	GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();

	m_meshRenderObject = m_renderScene->CreateRenderObject(m_mesh);
	m_meshRenderObject->SetTransform(transformComponent->GetWorldMatrix());
}

void GifChatComponent::GenerateMesh(const std::shared_ptr<MaterialInstance>& material)
{
	std::vector<StaticMesh::SubMesh> subMeshes;
	MeshGeometry& geo = subMeshes.emplace_back().Geometry;
	geo.SetName("GIF_RENDER_MESH");

	MeshVertex topLeft;
	topLeft.Position = Vector3D(-0.5f, 0.5f, 0.0f);
	topLeft.UVData = Vector2D(0.0f, 0.0f);

	MeshVertex topRight;
	topRight.Position = Vector3D(0.5f, 0.5f, 0.0f);
	topRight.UVData = Vector2D(1.0f, 0.0f);

	MeshVertex bottomLeft;
	bottomLeft.Position = Vector3D(-0.5f, -0.5f, 0.0f);
	bottomLeft.UVData = Vector2D(0.0f, 1.0f);

	MeshVertex bottomRight;
	bottomRight.Position = Vector3D(0.5f, -0.5f, 0.0f);
	bottomRight.UVData = Vector2D(1.0f, 1.0f);

	Vector3D top = topRight.Position - topLeft.Position;
	Vector3D left = topRight.Position - bottomLeft.Position;

	Vector3D planeNormal = top.Cross(left);

	topLeft.Normal = planeNormal;
	topRight.Normal = planeNormal;
	bottomLeft.Normal = planeNormal;
	bottomRight.Normal = planeNormal;

	geo.AddVertex(topLeft);  
	geo.AddVertex(topRight); 
	geo.AddVertex(bottomLeft);
	geo.AddVertex(bottomRight);

	// Counter-clockwise seen from the +Z side the normal faces
	geo.AddIndex(0);
	geo.AddIndex(2);
	geo.AddIndex(1);
	geo.AddIndex(1);
	geo.AddIndex(2);
	geo.AddIndex(3);

	
	geo.AllocateData();

	std::vector<StaticMesh::Slot> slots(1);
	slots[0].Name = material->GetName();
	slots[0].Material = material;

	m_mesh = StaticMeshInstance::Create(StaticMesh::Create("GIF_RENDER_MESH", std::move(subMeshes), std::move(slots)));
}

void GifChatComponent::DescribeProperties(PropertyBuilder<GifChatComponent>& properties)
{
	properties.Array("Materials", [](const GifChatComponent& gifChat)
		{
			const StaticMeshInstance* const mesh = gifChat.GetMesh();
			return mesh != nullptr ? MakeMaterialSlotRefs(*mesh) : std::vector<ObjectRef>();
		});
}
