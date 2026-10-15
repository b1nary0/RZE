#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/GraphicsDefines.h>
#include <Graphics/StaticMeshInstance.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <memory>

struct GifData;

class MaterialInstance;
class RenderObject;
class RenderScene;
class Texture2D;

class GifChatComponent : public GameObjectComponent<GifChatComponent>
{
public:
	GifChatComponent();
	~GifChatComponent() override;

	//
	// GameObjectComponent interface
	//
public:
	static void DescribeProperties(PropertyBuilder<GifChatComponent>& properties);

	void Initialize() override;
	void OnAddToScene() override;
	void OnRemoveFromScene() override;

	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

public:
	void Load(const Filepath& fp);

	// Null until a GIF has loaded
	const StaticMeshInstance* GetMesh() const { return m_mesh.get(); }

private:
	void CreateRenderObject();
	// A quad drawn with material
	void GenerateMesh(const std::shared_ptr<MaterialInstance>& material);

private:
	// The owner's scene's; set in Initialize, once the component has an owner, and fixed from then on since objects
	// never change scene
	RenderScene* m_renderScene = nullptr;

	int m_totalFrames;
	int m_currentDisplayingFrame = 0;
	float m_elapsedMS = 0.0f;
	std::vector<int> m_frameDelays;

	std::vector<ResourceHandle> m_frames;
	RenderObjectPtr m_meshRenderObject;
	std::shared_ptr<StaticMeshInstance> m_mesh;
	// The quad's material, kept to show each frame in turn; the mesh itself can't be changed once made
	std::shared_ptr<MaterialInstance> m_material;
};