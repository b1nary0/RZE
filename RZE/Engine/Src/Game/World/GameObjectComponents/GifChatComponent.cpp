#include <StdAfx.h>
#include <Game/World/GameObjectComponents/GifChatComponent.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/MeshComponentProperties.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/RenderScene.h>
#include <Graphics/Texture2D.h>


#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>

STBIDEF unsigned char* stbi_xload(stbi__context* s, int* x, int* y, int* frames, int** delays);
STBIDEF unsigned char* stbi_xload_mem(unsigned char* buffer, int len, int* x, int* y, int* frames, int** delays);
STBIDEF unsigned char* stbi_xload_file(char const* filename, int* x, int* y, int* frames, int** delays);

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

struct GifData
{
	unsigned char* m_data;
};

GifChatComponent::GifChatComponent()
{
}

GifChatComponent::~GifChatComponent()
{
}

void GifChatComponent::Initialize()
{
	m_renderScene = &GetOwner()->GetScene().GetRenderScene();

	Load(Filepath(gifPaths[GifFilepathIndex::PULP_FICTION_TRAVOLTA]));
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
	int x = 0;
	int y = 0;
	int* delays = nullptr;

	unsigned char* gifData = stbi_xload_file(fp.GetAbsolutePath().c_str(), &x, &y, &m_totalFrames, &delays);
	AssertNotNull(gifData);

	// Store array data in vector instead of holding raw ptr to data
	m_frameDelays.reserve(m_totalFrames);
	for (int frameIdx = 0; frameIdx < m_totalFrames; ++frameIdx)
	{
		m_frameDelays.push_back(delays[frameIdx]);
	}

	const size_t frameSizeBytes = (x * y) * 4;
	for (int frame = 0; frame < m_totalFrames; ++frame)
	{
		Texture2D* frameTexture = new Texture2D();

		// Each frame's texture frees its texels once they're uploaded, so it gets a copy of its slice of the GIF
		U8* const frameTexels = new U8[frameSizeBytes];
		memcpy(frameTexels, gifData + frame * frameSizeBytes, frameSizeBytes);
		frameTexture->Load(Rendering::TexelBuffer(frameTexels, &FreeFrameTexels), x, y);

		m_frames.push_back(RZE().GetResourceHandler().Make("FIRST_FRAME_GIF_" + std::to_string(frame), frameTexture));
		AssertExpr(m_frames.back().IsValid());
	}

	// Frames own copies of their data now
	stbi_image_free(gifData);

	ResourceHandle shader = 
		RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_NewRenderer_DiffuseOnly.hlsl"), "Pixel_NewRenderer_DiffuseOnly");
	AssertExpr(shader.IsValid());

	m_material = std::make_shared<MaterialInstance>("GifMaterial");
	m_material->SetTexture(0, m_frames[m_currentDisplayingFrame]);
	m_material->SetShaderTechnique(shader);

	GenerateMesh(m_material);

	STBI_FREE(delays);
}

void GifChatComponent::CreateRenderObject()
{
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

//
// STBI STUFF
// @TODO REMOVE SOON
//
STBIDEF unsigned char* stbi_xload_mem(unsigned char* buffer, int len, int* x, int* y, int* frames, int** delays)
{
	stbi__context s;
	stbi__start_mem(&s, buffer, len);
	return stbi_xload(&s, x, y, frames, delays);
}

STBIDEF unsigned char* stbi_xload_file(char const* filename, int* x, int* y, int* frames, int** delays)
{
	FILE* f;
	stbi__context s;
	unsigned char* result = 0;

	if (!(f = stbi__fopen(filename, "rb")))
		return stbi__errpuc("can't fopen", "Unable to open file");

	stbi__start_file(&s, f);
	result = stbi_xload(&s, x, y, frames, delays);
	fclose(f);

	return result;
}

STBIDEF unsigned char* stbi_xload(stbi__context* s, int* x, int* y, int* frames, int** delays)
{
	int comp;
	unsigned char* result = 0;

	if (stbi__gif_test(s))
		return static_cast<unsigned char*>(stbi__load_gif_main(s, delays, x, y, frames, &comp, 4));

	stbi__result_info ri;
	result = static_cast<unsigned char*>(stbi__load_main(s, x, y, &comp, 4, &ri, 8));
	*frames = !!result;

	if (ri.bits_per_channel != 8) {
		STBI_ASSERT(ri.bits_per_channel == 16);
		result = stbi__convert_16_to_8((stbi__uint16*)result, *x, *y, 4);
		ri.bits_per_channel = 8;
	}

	return result;
}

void GifChatComponent::DescribeProperties(PropertyBuilder<GifChatComponent>& properties)
{
	properties.Array("Materials", [](const GifChatComponent& gifChat)
		{
			const StaticMeshInstance* const mesh = gifChat.GetMesh();
			return mesh != nullptr ? MakeMaterialSlotRefs(*mesh) : std::vector<ObjectRef>();
		});
}
