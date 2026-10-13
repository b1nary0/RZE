#include <StdAfx.h>
#include <Game/GameScene.h>

#include <Game/SceneFormat.h>
#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>
#include <unordered_set>

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
}

void GameScene::Initialize()
{
}

void GameScene::NewScene()
{
	Deserialize(GetDefaultScenePath());
}

const Filepath& GameScene::GetDefaultScenePath()
{
	static const Filepath s_defaultScenePath("Assets/Scenes/Default.scene");
	return s_defaultScenePath;
}

void GameScene::Serialize(const Filepath& filePath)
{
	std::vector<GameObjectPtr> roots;
	for (auto& gameObject : m_objectRegistry)
	{
		if (gameObject->IsInScene() && gameObject->IncludeInSave() && gameObject->IsRoot())
		{
			roots.push_back(GameObjectPtr(gameObject.get()));
		}
	}

	rapidjson::StringBuffer buf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
	SceneFormat::Write(writer, roots);
	
	File sceneFile;
	if (filePath.IsValid())
	{
		sceneFile.SetFilePath(filePath.GetRelativePath());
		mCurrentScenePath = filePath;
	}
	else
	{
		sceneFile.SetFilePath(mCurrentScenePath.GetRelativePath());
	}

	sceneFile.Open(File::EFileOpenMode::Write);
	sceneFile.Write(buf.GetString());
	AssertExpr(sceneFile.IsValid());
	sceneFile.Close();
}

void GameScene::Deserialize(const Filepath& filePath)
{
	mCurrentScenePath = filePath;

	rapidjson::Document sceneDoc;
	std::string error;
	if (!ParseSceneFile(filePath, sceneDoc, error))
	{
		RZE_LOG_ARGS("Failed to load scene: %s", error.c_str());
		return;
	}

	GameObjectLoader loader(*this);
	for (const rapidjson::Value& entry : SceneFormat::GetObjects(sceneDoc).GetArray())
	{
		loader.Load(entry);
	}

	ResolveMainCamera();
}

bool GameScene::ParseSceneFile(const Filepath& filePath, rapidjson::Document& outDocument, std::string& outError)
{
	if (!filePath.IsValid() || !filePath.Exists())
	{
		outError = "Scene file [" + filePath.GetRelativePath() + "] does not exist.";
		return false;
	}

	File sceneFile(filePath);
	sceneFile.Read();
	sceneFile.Close();

	if (sceneFile.Content().empty())
	{
		outError = "Scene file [" + filePath.GetRelativePath() + "] could not be read or is empty.";
		return false;
	}

	outDocument.Parse(sceneFile.Content().c_str());
	if (outDocument.HasParseError() || !outDocument.IsObject())
	{
		outError = "Scene file [" + filePath.GetRelativePath() + "] is not valid JSON (error at offset " + std::to_string(outDocument.GetErrorOffset()) + ").";
		return false;
	}

	std::string formatError;
	if (!SceneFormat::Upgrade(outDocument, formatError))
	{
		outError = "Scene file [" + filePath.GetRelativePath() + "] can't be read: " + formatError + ".";
		return false;
	}

	return true;
}

GameObjectPtr GameScene::DeserializeGameObject(const rapidjson::Value& entry, GameObjectPtr parent)
{
	std::unique_ptr<GameObject> gameObject = CreateGameObjectNoComponents(ChooseGUID(entry));
	gameObject->SetName(entry["name"].GetString());
	gameObject->LoadComponents(entry);
	if (parent != nullptr)
	{
		gameObject->AttachTo(parent);
	}

	GameObjectPtr created(gameObject.get());
	AddGameObject(std::move(gameObject));
	return created;
}

std::unique_ptr<GameObject> GameScene::CreateGameObjectNoComponents(ObjectGUID guid)
{
	return std::make_unique<GameObject>(guid);
}

ObjectGUID GameScene::ChooseGUID(const rapidjson::Value& data) const
{
	// Scenes saved before GUIDs have no "id"; a pasted copy of an object still in the scene has a taken one
	const auto id = data.FindMember("id");
	const ObjectGUID saved = id != data.MemberEnd() && id->value.IsString() ? ObjectGUIDs::FromString(id->value.GetString()) : ObjectGUIDs::k_invalid;
	if (saved != ObjectGUIDs::k_invalid && m_objectsByGUID.find(saved) == m_objectsByGUID.end())
	{
		return saved;
	}

	return GenerateFreeGUID();
}

ObjectGUID GameScene::GenerateFreeGUID() const
{
	// A clash between random 64-bit values is vanishingly unlikely, but cheap to rule out
	ObjectGUID guid = ObjectGUIDs::Generate();
	while (m_objectsByGUID.find(guid) != m_objectsByGUID.end())
	{
		guid = ObjectGUIDs::Generate();
	}
	return guid;
}

void GameScene::Unload()
{
	ProcessObjectRemoveDeferrals();

	for (auto& gameObject : m_objectRegistry)
	{
		// Some may already be out of the scene, set aside by a tool
		if (gameObject->IsInScene())
		{
			gameObject->OnRemoveFromScene();
		}
		gameObject->Uninitialize();
	}

	m_objectRegistry.clear();
	m_objectsByGUID.clear();
	m_mainCamera = nullptr;
}

void GameScene::AddGameObject(std::unique_ptr<GameObject>&& gameObject)
{
	// Objects are identified by GUID, not name, so names can repeat. GameScene only creates objects with free GUIDs
	// (ChooseGUID, GenerateFreeGUID), so a taken one means this object is already in the scene.
	const bool isGUIDFree = m_objectsByGUID.emplace(gameObject->GetGUID(), gameObject.get()).second;
	AssertMsg(isGUIDFree, "GameObject already exists in scene");
	if (!isGUIDFree)
	{
		return;
	}

	gameObject->OnAddToScene();
	m_objectRegistry.emplace_back(std::move(gameObject));

	if (m_onObjectsChanged)
	{
		m_onObjectsChanged();
	}
}

void GameScene::AddGameObject(const GameObjectPtr& gameObject)
{
	AddGameObject(std::unique_ptr<GameObject>(gameObject.m_ptr));
}

void GameScene::ProcessObjectRemoveDeferrals()
{
	if (m_objectsToRemove.empty())
	{
		return;
	}

	// One pass over the registry for every removal this frame, keeping the order of what's left
	std::unordered_set<const GameObject*> toRemove;
	for (GameObjectPtr& gameObject : m_objectsToRemove)
	{
		AssertExpr(!gameObject->IsInScene());
		toRemove.insert(*gameObject);
	}
	m_objectsToRemove.clear();

	for (std::unique_ptr<GameObject>& gameObject : m_objectRegistry)
	{
		if (toRemove.count(gameObject.get()) != 0)
		{
			// @TODO This shouldnt be here but we'll end up with "leaking" objects when
			// we remove from the scene but dont call Uninitialize (which stray resources will be stale and bad)
			gameObject->Uninitialize();
			m_objectsByGUID.erase(gameObject->GetGUID());
		}
	}

	m_objectRegistry.erase(std::remove_if(m_objectRegistry.begin(), m_objectRegistry.end(),
		[&toRemove](const std::unique_ptr<GameObject>& gameObject)
		{
			return toRemove.count(gameObject.get()) != 0;
		}), m_objectRegistry.end());
}

void GameScene::RemoveGameObject(GameObjectPtr& gameObject)
{
	AssertNotNull(gameObject);

	// Out of the hierarchy, so the parent doesn't keep a child that's about to be destroyed
	if (gameObject->GetParent() != nullptr)
	{
		gameObject->DetachFromParent();
	}

	std::vector<GameObject*> subtree;
	gameObject->GetSubtree(subtree);
	for (GameObject* object : subtree)
	{
		// It may already be out of the scene, set aside by a tool
		if (object->IsInScene())
		{
			object->OnRemoveFromScene();
		}
		m_objectsToRemove.emplace_back(GameObjectPtr(object));
	}
	gameObject = GameObjectPtr();

	if (m_onObjectsChanged)
	{
		m_onObjectsChanged();
	}
}

GameObjectPtr GameScene::FindGameObjectByName(const std::string& name)
{
	// A scan, since names aren't indexed; prefer FindGameObjectByGUID to find a specific object
	auto iter = std::find_if(m_objectRegistry.begin(), m_objectRegistry.end(),
		[&name](const std::unique_ptr<GameObject>& object)
		{
			return object->IsInScene() && object->GetName() == name;
		});

	if (iter != m_objectRegistry.end())
	{
		return GameObjectPtr((*iter).get());
	}

	return nullptr;
}

GameObjectPtr GameScene::FindGameObjectByGUID(ObjectGUID guid)
{
	const auto it = m_objectsByGUID.find(guid);
	if (it == m_objectsByGUID.end() || !it->second->IsInScene())
	{
		return GameObjectPtr();
	}
	return GameObjectPtr(it->second);
}

GameObjectPtr GameScene::AddGameObject(const std::string& name)
{
	GameObjectPtr gameObject = CreateGameObject();
	
	gameObject->SetName(name);

	AddGameObject(gameObject);

	return gameObject;
}

void GameScene::ForEachGameObject(Functor<void, GameObjectPtr> func)
{
	for (auto& gameObject : m_objectRegistry)
	{
		if (gameObject->IsInScene())
		{
			func(GameObjectPtr(gameObject.get()));
		}
	}
}

GameObjectPtr GameScene::CreateGameObject()
{
	std::unique_ptr<GameObject> gameObject = std::make_unique<GameObject>(GenerateFreeGUID());
	gameObject->AddComponent<TransformComponent>();
	gameObject->Initialize();

	return GameObjectPtr(gameObject.release());
}

GameObjectPtr GameScene::GetMainCamera() const
{
	// Null from when it's gone until ReplaceMainCameraIfGone picks another
	return IsMainCameraGone() ? nullptr : m_mainCamera;
}

bool GameScene::IsMainCameraGone() const
{
	return m_mainCamera == nullptr || !m_mainCamera->IsInScene() || m_mainCamera->GetComponent<CameraComponent>() == nullptr;
}

void GameScene::SetMainCamera(const GameObjectPtr& camera)
{
	AssertMsg(camera == nullptr || camera->GetComponent<CameraComponent>() != nullptr, "The main camera needs a CameraComponent");

	for (auto& gameObject : m_objectRegistry)
	{
		GameObjectComponentPtr<CameraComponent> cameraComponent = gameObject->GetComponent<CameraComponent>();
		if (cameraComponent != nullptr)
		{
			cameraComponent->m_isMainCamera = camera != nullptr && GameObjectPtr(gameObject.get()) == camera;
		}
	}

	m_mainCamera = camera;
}

void GameScene::ResolveMainCamera()
{
	GameObjectPtr firstCamera;
	GameObjectPtr firstMain;
	for (auto& gameObject : m_objectRegistry)
	{
		GameObjectComponentPtr<CameraComponent> camera = gameObject->GetComponent<CameraComponent>();
		if (!gameObject->IsInScene() || camera == nullptr)
		{
			continue;
		}

		if (firstCamera == nullptr)
		{
			firstCamera = GameObjectPtr(gameObject.get());
		}

		if (camera->IsMainCamera())
		{
			if (firstMain == nullptr)
			{
				firstMain = GameObjectPtr(gameObject.get());
			}
			else
			{
				RZE_LOG_ARGS("Scene has more than one main camera; using [%s], not [%s].", firstMain->GetName().c_str(), gameObject->GetName().c_str());
			}
		}
	}

	SetMainCamera(firstMain != nullptr ? firstMain : firstCamera);
}

void GameScene::ReplaceMainCameraIfGone()
{
	// Nothing to replace if there was never one, or it was destroyed (unload resets it)
	if (m_mainCamera == nullptr || !IsMainCameraGone())
	{
		return;
	}

	const std::string previousName = m_mainCamera->GetName();

	// Skips the camera that's gone, since it's out of the scene or no longer a camera
	ResolveMainCamera();

	if (m_mainCamera != nullptr)
	{
		RZE_LOG_ARGS("Main camera [%s] is gone; [%s] is the main camera now.", previousName.c_str(), m_mainCamera->GetName().c_str());
	}
	else
	{
		RZE_LOG_ARGS("Main camera [%s] is gone, and there's no other camera to replace it.", previousName.c_str());
	}
}

Vector3D GameScene::CalculateSceneCenter() const
{
	bool hasRenderedObject = false;
	Vector3D boundsMin;
	Vector3D boundsMax;

	for (const auto& gameObject : m_objectRegistry)
	{
		if (!gameObject->IsInScene() || !gameObject->IsRoot() || gameObject->GetComponent<RenderComponent>() == nullptr)
		{
			continue;
		}

		const Vector3D& position = gameObject->GetTransformComponent()->GetPosition();
		if (!hasRenderedObject)
		{
			boundsMin = position;
			boundsMax = position;
			hasRenderedObject = true;
			continue;
		}

		boundsMin = Vector3D(std::min(boundsMin.X(), position.X()), std::min(boundsMin.Y(), position.Y()), std::min(boundsMin.Z(), position.Z()));
		boundsMax = Vector3D(std::max(boundsMax.X(), position.X()), std::max(boundsMax.Y(), position.Y()), std::max(boundsMax.Z(), position.Z()));
	}

	return (boundsMin + boundsMax) * 0.5f;
}

void GameScene::Start()
{
}

void GameScene::Update()
{
	OPTICK_EVENT();
	// Before the removals, while a removed main camera is still around to name
	ReplaceMainCameraIfGone();
	ProcessObjectRemoveDeferrals();

	for (auto& gameObject : m_objectRegistry)
	{
		// @todo eventually we should _only_ store root objects maybe?
		// feels like there could be some gross maintaining that during
		// AddChild/RemoveChild calls...
		if (gameObject->IsInScene() && gameObject->IsRoot())
		{
			gameObject->Update();
		}
	}
}

void GameScene::ShutDown()
{
	ProcessObjectRemoveDeferrals();

	for (auto& gameObject : m_objectRegistry)
	{
		gameObject->Uninitialize();
	}
	m_objectRegistry.clear();
	m_objectsByGUID.clear();
	m_mainCamera = nullptr;
}