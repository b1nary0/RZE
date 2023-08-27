#include <StdAfx.h>
#include <Game/World/GameObjectComponents/GifChatComponent.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/RenderEngine.h>
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

	Filepath gifFilepaths[] =
	{
		Filepath("Assets/2D/GIFs/friends-joey-tribbiani.gif"),
		Filepath("Assets/2D/GIFs/pulp_fiction_travolta.gif")
	};

	std::string GetTextureTypeStr(MaterialInstance::TextureSlot textureSlot)
	{
		switch (textureSlot)
		{
		case MaterialInstance::TEXTURE_SLOT_DIFFUSE:
			return "Diffuse";
		case MaterialInstance::TEXTURE_SLOT_SPECULAR:
			return "Specular";
		case MaterialInstance::TEXTURE_SLOT_NORMAL:
			return "Normal";
		default:
			return "None";
		}
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
	Load(gifFilepaths[GifFilepathIndex::PULP_FICTION_TRAVOLTA]);
}

void GifChatComponent::OnAddToScene()
{
	CreateRenderObject();
}

void GifChatComponent::OnRemoveFromScene()
{
	if (m_meshRenderObject != nullptr)
	{
		RZE().GetRenderEngine().DestroyRenderObject(m_meshRenderObject);
	}
}

void GifChatComponent::Update()
{
	static float elapsedMS = 0.0f;
	elapsedMS += RZE().GetDeltaTimeMS();

	if (m_currentDisplayingFrame < m_totalFrames)
	{
		if (elapsedMS >= static_cast<float>(m_frameDelays[m_currentDisplayingFrame]))
		{
			m_meshGeometry.GetSubMeshes()[0].GetMaterial()->SetTexture(0, m_frames[m_currentDisplayingFrame]);
			++m_currentDisplayingFrame;

			elapsedMS = 0;
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
		m_meshRenderObject->SetTransform(transformComponent->GetAsMat4x4());
	}
}

void GifChatComponent::OnEditorInspect()
{
	for (const auto& subMesh : m_meshGeometry.GetSubMeshes())
	{
		if (ImGui::TreeNode(&subMesh, subMesh.GetName().c_str()))
		{
			const std::shared_ptr<const MaterialInstance>& material = subMesh.GetMaterial();
			//
			// Material Information
			//
			if (ImGui::TreeNode(&material, "%s %s", "Material:", material->GetName().c_str()))
			{
				const PixelShader* const shader = RZE().GetResourceHandler().GetResource<PixelShader>(material->GetShaderResource());
				AssertNotNull(shader);

				ImGui::Separator();

				ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "[Shader] ");
				ImGui::Text(shader->GetName().c_str());

				ImGui::NewLine();

				//
				// Textures
				//
				ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "[Textures]");
				for (U8 textureSlot = 0; textureSlot < MaterialInstance::TEXTURE_SLOT_COUNT; ++textureSlot)
				{
					const ResourceHandle& textureResource = material->GetTexture(textureSlot);

					if (textureResource.IsValid())
					{
						const Texture2D* const textureData = RZE().GetResourceHandler().GetResource<Texture2D>(textureResource);
						AssertNotNull(textureData);

						const std::string textureTypeStr = GetTextureTypeStr(static_cast<MaterialInstance::TextureSlot>(textureSlot));
						if (ImGui::CollapsingHeader(textureTypeStr.c_str()))
						{
							ImGui::Text(textureData->GetFilepath().GetRelativePath().c_str());
							ImGui::Image(textureData->GetPlatformObject().GetTextureData(), ImVec2(textureData->GetDimensions().X(), textureData->GetDimensions().Y()));
						}
					}
				}
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
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
	m_gifData = std::unique_ptr<unsigned char>(gifData);
	AssertNotNull(m_gifData);

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

		// Copy the texture data into a buffer to pass to Texture2D to take ownership of
		// this is necessary atm because there is no "Instance" layer of resources - just the immutable
		// resources themselves from ResourceHandler. Since this hacky meme implementation needs to circumvent that
		// instead of writing a whole new architecture layer, we can't be deleting memory (as will happen when the resource
		// releases) which we don't own. Essentially we can't allow strong ownership of any memory slice of m_gifData
		// because of this exploitation of the resource system.
		// #TODO
		// Make ClassDef and ClassInstance to support mutable resources?
		U8* textureBuffer = new U8[frameSizeBytes];
		memcpy(textureBuffer, m_gifData.get() + frame * frameSizeBytes, frameSizeBytes);
		frameTexture->Load(textureBuffer, x, y);
		
		m_frames.push_back(RZE().GetResourceHandler().Make("FIRST_FRAME_GIF_" + std::to_string(frame), frameTexture));
		AssertExpr(m_frames.back().IsValid());
	}

	GenerateMesh();

	ResourceHandle shader = 
		RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_NewRenderer_DiffuseOnly.hlsl"), "Pixel_NewRenderer_DiffuseOnly");
	AssertExpr(shader.IsValid());

	std::shared_ptr<MaterialInstance> material = std::make_shared<MaterialInstance>("GifMaterial");
	material->SetTexture(0, m_frames[m_currentDisplayingFrame]);
	material->SetShaderTechnique(shader);

	// For purposes, always one mesh - a quad
	MeshGeometry& geo = m_meshGeometry.GetSubMeshes()[0];
	geo.SetMaterial(material);

	delete delays;
}

void GifChatComponent::CreateRenderObject()
{
	GameObjectComponentPtr<TransformComponent> transformComponent = GetOwner()->GetComponent<TransformComponent>();

	m_meshRenderObject = RZE().GetRenderEngine().CreateRenderObject(m_meshGeometry);
	m_meshRenderObject->SetTransform(transformComponent->GetAsMat4x4());
}

void GifChatComponent::GenerateMesh()
{
	StaticMeshInstance mesh;

	std::vector<MeshGeometry> geometryList;
	MeshGeometry& geo = geometryList.emplace_back();
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

	geo.AddIndex(0);
	geo.AddIndex(1);
	geo.AddIndex(2);
	geo.AddIndex(1);
	geo.AddIndex(3);
	geo.AddIndex(2);

	
	geo.AllocateData();

	mesh.Initialize(std::move(geometryList));

	m_meshGeometry = std::move(mesh);
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