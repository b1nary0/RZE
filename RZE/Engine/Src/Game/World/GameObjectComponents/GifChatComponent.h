#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/StaticMesh.h>

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

	void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Load(const rapidjson::Value& data) override;

public:
	void Load(const Filepath& fp);

private:
	void CreateRenderObject();
	void GenerateMesh();

private:
	int m_totalFrames;
	int m_currentDisplayingFrame = 0;
	std::vector<int> m_frameDelays;

	std::unique_ptr<GifData> m_gifData;
	std::vector<ResourceHandle> m_frames;
	std::shared_ptr<RenderObject> m_meshRenderObject;
	StaticMesh m_meshGeometry;
};