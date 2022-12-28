#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/GraphicsDefines.h>
#include <Graphics/StaticMeshInstance.h>

struct GifData;

class RenderObject;
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
	void Initialize() override;
	void OnAddToScene() override;
	void OnRemoveFromScene() override;

	void Update() override;

	void OnEditorInspect() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

public:
	void Load(const Filepath& fp);

private:
	void CreateRenderObject();
	void GenerateMesh();

private:
	int m_totalFrames;
	int m_currentDisplayingFrame = 0;
	std::vector<int> m_frameDelays;

	std::unique_ptr<unsigned char> m_gifData;
	std::vector<ResourceHandle> m_frames;
	RenderObjectPtr m_meshRenderObject;
	StaticMeshInstance m_meshGeometry;
};