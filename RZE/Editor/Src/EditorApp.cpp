#include <EditorApp.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObject/EditorGameComponentInfoCache.h>
#include <Game/World/GameObjectComponents/CameraComponent.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/RenderComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Rendering/Renderer.h>
#include <Rendering/MemArena.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Graphics/RenderEngine.h>
#include <Graphics/RenderStages/ImGuiRenderStage.h>

#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <Utils/Memory/MemoryUtils.h>
#include <Utils/Platform/CmdLine.h>
#include <Utils/Platform/Filepath.h>

#include <DebugUtils/DebugServices.h>

#include <ImGui/imgui.h>

#include <Optick/optick.h>

#include <stdio.h>

namespace
{
	const Filepath k_sceneFileDirectoryPath("Assets/Scenes");

	constexpr char kSceneFileToLoadHack[] = { "Assets/Scenes/RenderTest.scene" };

	// #TODO
	// Uber temp struct here to represent editor state.
	// Needs to move out eventually into something not bad.
	struct EditorState
	{
		bool IsNewScene = true;
	};
}

namespace Editor
{
	static EditorState gEditorState;

	EditorApp::EditorApp()
	{
		REGISTER_GAMEOBJECTCOMPONENT(EditorCameraComponent);

		// 0x0100 is the default, so we put the transform component decently high up.
		REGISTER_EDITORCOMPONENTCACHE_ORDERDATA( TransformComponent, 0x00F0 );

		// This would be first if uncommented.
		//REGISTER_EDITORCOMPONENTCACHE_ORDERDATA( RenderComponent, 0x000F );

		// Other examples...
		//REGISTER_EDITORCOMPONENTCACHE_ORDERDATA( EditorCameraComponent, 0x0300 );
		//REGISTER_EDITORCOMPONENTCACHE_ORDERDATA( CameraComponent, 0x000E );
	}

	EditorApp::~EditorApp()
	{
	}

	void EditorApp::Initialize()
	{
		RZE_Application::Initialize();

		const bool isWithEditor = true;
		RZE().GetRenderEngine().AddRenderStage<ImGuiRenderStage>(isWithEditor);

		m_imguiConfigFilepath = Filepath("Config/imgui.ini");
		ImGui::GetIO().IniFilename = m_imguiConfigFilepath.GetAbsolutePath().c_str();

		GetWindow()->SetTitle("RZEStudio");
		GetWindow()->Maximize();

		const Vector2D& windowDims = GetWindow()->GetClientSize();
		CreateRenderTarget(windowDims);
		RZE().GetRenderEngine().SetRenderTarget(m_renderTarget.get());

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::GetIO().KeyRepeatDelay = 0.5f;

		LoadFonts();
		StyleSetup();
	}

	void EditorApp::Start()
	{
		// #TODO
		// Need to write some code that better defines loading into the last scene that was loaded
		// when the editor closed. This is the difference between fresh (I've loaded into a new scene but havent saved)
		// and "stale" (I closed the editor within a scene and it needs to be loaded for UX reasons).
		// #NOTE
		// For now, it's always false here because we always load into TestGame.scene
		gEditorState.IsNewScene = false;

		std::string_view startScene;
		Filepath scenePath;
		if (CmdLine::Arguments::Get("-scene", startScene))
		{
			scenePath = Filepath("Assets/Scenes/" + std::string(startScene));
			LoadScene(scenePath);
		}
		else
		{
			scenePath = Filepath(kSceneFileToLoadHack);
			LoadScene(scenePath);
		}

		AddFilePathToWindowTitle(scenePath.GetRelativePath());
	}

	void EditorApp::Update()
	{
		OPTICK_EVENT();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		Vector2D clientSize = GetWindow()->GetClientSize();

		ImGui::PushFont(m_fontMapping.at("din_bold"));
		DisplayMenuBar();
		ImGui::SetNextWindowPos(ImVec2(0.f, 24.0f));
		ImGui::SetNextWindowSize(ImVec2(clientSize.X(), clientSize.Y() - 24.0f));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		bool show = true;
		ImGui::Begin("RZE_Editor", &show, window_flags);
		ImGui::PopStyleVar(1);
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

			ResolvePanelState();
		}
		ImGui::PopFont();
		ImGui::End();
	}

	void EditorApp::ShutDown()
	{
	}

	void EditorApp::RegisterInputEvents(InputHandler& inputHandler)
	{
		// NOTE: This will only get called when we're hovering the SceneView panel
		// SEE: EditorApp::ProcessInput return statement. 
		Functor<void, const InputKey&> keyFunc([this](const InputKey& key)
			{
				if (key.GetKeyCode() == Win32KeyCode::Escape)
				{
					if (GetSelectedObjectFromScenePanel() != nullptr)
					{
						ResetSelectedObject();
					}
				}
			});
		inputHandler.BindAction(Win32KeyCode::Escape, EButtonState::ButtonState_Pressed, keyFunc);
	}

	bool EditorApp::ProcessInput(const InputHandler& handler)
	{
		ImGuiIO& io = ImGui::GetIO();

		const Vector2D& mousePos = handler.GetProxyMouseState().CurPosition;
		const Vector2D& prevMousePos = handler.GetProxyMouseState().PrevPosition;
		io.MousePos = ImVec2(mousePos.X(), mousePos.Y());
		io.MousePosPrev = ImVec2(prevMousePos.X(), prevMousePos.Y());

		for (U32 mouseBtn = 0; mouseBtn < 3; ++mouseBtn)
		{
			io.MouseDown[mouseBtn] = handler.GetProxyMouseState().CurMouseBtnStates[mouseBtn];
		}

		for (int key = 0; key < MAX_KEYCODES_SUPPORTED; ++key)
		{
			io.KeysDown[key] = handler.GetProxyKeyboardState().IsDownThisFrame(key);
		}

		io.KeyCtrl = handler.GetProxyKeyboardState().IsDownThisFrame(Win32KeyCode::Control);
		// @TODO this is commented because imgui needs extra data or im doing something wrong
		// to not scroll to the very top or bottom every mouse notch.
		//io.MouseWheel = static_cast<float>(handler.GetProxyMouseState().CurWheelVal);

		return m_sceneViewPanel.IsHovered();
	}

	void EditorApp::OnWindowResize(const Vector2D& newSize)
	{
		CreateRenderTarget(newSize);
		RZE().GetRenderEngine().SetRenderTarget(m_renderTarget.get());
	}

	GameObjectPtr EditorApp::GetSelectedObjectFromScenePanel()
	{
		if (m_scenePanel.HasSelectedGameObject())
		{
			return m_scenePanel.GetSelectedGameObject();
		}

		return GameObjectPtr();
	}

	void EditorApp::ResetSelectedObject()
	{
		m_scenePanel.ResetSelectedGameObject();
	}

	void EditorApp::CreateRenderTarget(const Vector2D& dimensions)
	{
		m_renderTarget.reset();

		m_renderTarget = std::make_unique<Rendering::RenderTargetTexture>(static_cast<U32>(dimensions.X()), static_cast<U32>(dimensions.Y()));
		m_renderTarget->Initialize();
	}

	void EditorApp::SetFont(const char* fontName)
	{
		ImGui::PopFont();
		ImGui::PushFont(m_fontMapping.at(fontName));
	}

	void EditorApp::Log(const std::string& msg)
	{
		m_logPanel.AddEntry(msg);
	}

	void EditorApp::CreateAndInitializeEditorCamera()
	{
		m_editorCameraObject = RZE().GetActiveScene().AddGameObject("EditorCam");
		GameObjectComponentPtr<TransformComponent> transformComponent = m_editorCameraObject->GetComponent<TransformComponent>();
		transformComponent->SetPosition(Vector3D(-4.0f, 10.0f, 4.0f));

		GameObjectComponentPtr<EditorCameraComponent> editorCam = m_editorCameraObject->AddComponent<EditorCameraComponent>();
		editorCam->SetAsActiveCamera(true);
		const Vector2D& sceneViewPanelDims = m_sceneViewPanel.GetDimensions();
		editorCam->SetAspectRatio(sceneViewPanelDims.X() / sceneViewPanelDims.Y());
	}

	void EditorApp::DisplayMenuBar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 5.0f));
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene..."))
				{
					gEditorState.IsNewScene = true;

					LoadScene(Filepath());
				}
				if (ImGui::MenuItem("Load Scene..."))
				{
					FilePromptParams openFileParams =
					{
						"RZE Scene",
						k_sceneFileDirectoryPath,
						"*.scene"
					};

					std::string chosenPath;
					bool openSuccess = RZE().ShowOpenFilePrompt(openFileParams, chosenPath);
					if (openSuccess)
					{
						Filepath newScenePath = Filepath::FromAbsolutePathStr(chosenPath);
						LoadScene(newScenePath);

						AddFilePathToWindowTitle(newScenePath.GetRelativePath());
					}
				}
				if (ImGui::MenuItem("Save Scene"))
				{
					if (gEditorState.IsNewScene)
					{
						FilePromptParams promptParams =
						{
							"Save Scene",
							k_sceneFileDirectoryPath,
							"*.scene"
						};

						std::string chosenPath;
						bool saveSuccess = GetWindow()->ShowSaveFilePrompt(promptParams, chosenPath);
						if (saveSuccess)
						{
							Filepath newScenePath = Filepath::FromAbsolutePathStr(chosenPath);
							RZE().GetActiveScene().Serialize(newScenePath);
							gEditorState.IsNewScene = false;
							RunAssetCpy();
						}
					}
					else
					{
						RZE().GetActiveScene().Serialize(Filepath());
						RunAssetCpy();
					}
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Build Game..."))
				{
					DebugServices::Get().Trace(LogChannel::Build, "Building Game...");
					Threading::Job::Task buildTask([this]()
						{
							char buffer[2048];
							FILE* pipe = nullptr;
							static Filepath buildGameBat("BuildGame.bat");
							pipe = _popen(buildGameBat.GetAbsolutePath().c_str(), "rt");
							while (fgets(buffer, 2048, pipe))
							{
								Log(buffer);
							}
						});
					Threading::JobScheduler::Get().PushJob(buildTask);
				}
				if (ImGui::MenuItem("Launch Game..."))
				{
					Threading::Job::Task gameTask([this]()
						{
							static Filepath buildGameBat("BuildGame.bat");
							static Filepath gamePath("_Build\\Debug\\x64\\RZE_Game.exe");

							// #TODO
							// Make function to do this stuff
							{
								FILE* pipe = nullptr;
								pipe = _popen(buildGameBat.GetAbsolutePath().c_str(), "rt");
								char buffer[2048];
								while (fgets(buffer, 2048, pipe))
								{
									DebugServices::Get().Trace(LogChannel::Build, buffer);
								}
							}
							{
								RunAssetCpy();
							}
							{
								FILE* pipe = nullptr;
								pipe = _popen(gamePath.GetAbsolutePath().c_str(), "rt");
							}
						});
					Threading::JobScheduler::Get().PushJob(gameTask);

				}
				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
				{
					RZE().PostExit();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::BeginMenu("Windows"))
				{
					if (ImGui::MenuItem("Scene"))
					{
						m_scenePanel.Enable();
					}

					// UX note, having an exposed bool allows ImGui to display checkmarks in the menu on items in the dropdown.
					ImGui::MenuItem("Resource Monitor", "", &m_resourceMonitor.IsEnabled);

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("Show ImGui Demo Window"))
				{
					m_panelStates.bDemoPanelEnabled = true;
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();
			static int frameCount = 0;
			static float frametimeMS = 0.0f;
			if (frameCount > 15)
			{
				frametimeMS = RZE().GetDeltaTimeMS();
				frameCount = 0;
			}
			else
			{
				++frameCount;
			}

			const float imguiCursorPos = ImGui::GetCursorPosX();
			ImGui::Text("%.1f ms", frametimeMS);
			
			ImGui::Separator();
			{
				// @todo Move to function later maybe
				ImVec4 colour;
				Rendering::MemArena::PressureValue memPressure = Rendering::MemArena::GetPressureValue();
				switch (memPressure)
				{
				case Rendering::MemArena::PressureValue::LOW:
					colour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					break;
					
				case Rendering::MemArena::PressureValue::MED:
					colour = ImVec4(255.0f, 255.0f, 0.0f, 1.0f);
					break;

				case Rendering::MemArena::PressureValue::HIGH:
					colour = ImVec4(255.0f, 0.0f, 0.0f, 1.0f);
					break;

				default:
					colour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					break;
				}

				const size_t lastFrameAllocationSize = Rendering::MemArena::GetTotalMemoryAllocatedLastFrame();
				const size_t memArenaTotalSizeBytes = Rendering::MemArena::GetSize();

				const float lastFrameSizeKB = static_cast<float>(lastFrameAllocationSize / 1024);
				const float memArenaSizeKB = static_cast<float>(memArenaTotalSizeBytes / 1024);
				const float memArenaSizeMB = static_cast<float>(memArenaTotalSizeBytes / 1024) / 1024;

				const float* memArenaValue = &memArenaSizeKB;

				char* memArenaSizeMagnitudeStr = "KB";
				if (memArenaTotalSizeBytes >= MemoryUtils::Megabytes(1))
				{
					memArenaSizeMagnitudeStr = "MB";
					memArenaValue = &memArenaSizeMB;
				}

				ImVec2 textSize = ImGui::CalcTextSize("12.0ms ");
				ImGui::SameLine(imguiCursorPos + textSize.x);
				ImGui::Text("RenderCommand MemArena:");
				ImGui::TextColored(colour, "%.2f KB / %.2f %s", lastFrameSizeKB, *memArenaValue, memArenaSizeMagnitudeStr);
				ImGui::Text("Peak Used: %d", Rendering::MemArena::GetPeakUsedBytes() / 1024);
			}

			ImGui::EndMainMenuBar();
		}
		ImGui::PopStyleVar();
	}

	void EditorApp::HandleGeneralContextMenu()
	{
		if (ImGui::BeginPopupContextWindow("Scene"))
		{
			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::BeginMenu("Windows"))
				{
					if (ImGui::MenuItem("Scene"))
					{
						m_scenePanel.Enable();
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}
	}

	void EditorApp::ResolvePanelState()
	{
		if (m_panelStates.bDemoPanelEnabled)
		{
			ImGui::ShowDemoWindow(nullptr);
		}

		m_scenePanel.Display();

		m_sceneViewPanel.Display();

		m_resourceMonitor.Display();

		m_logPanel.Display();
	}

	void EditorApp::LoadFonts()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();

		Filepath ubuntuRegularPath("Assets/Fonts/Ubuntu-Regular.ttf");
		Filepath ubuntuMediumPath("Assets/Fonts/Ubuntu-Medium.ttf");
		Filepath arialPath("Assets/Fonts/Arial.ttf");
		Filepath consolasPath("Assets/Fonts/Consolas.ttf");
		Filepath liberationRegularPath("Assets/Fonts/LiberationMono-Bold.ttf");
		Filepath dinBoldPath("Assets/Fonts/D-DIN-Bold.otf");

		m_fontMapping.insert({ "ubuntu_medium", io.Fonts->AddFontFromFileTTF(ubuntuMediumPath.GetAbsolutePath().c_str(), 16) });
		m_fontMapping.insert({ "ubuntu_regular", io.Fonts->AddFontFromFileTTF(ubuntuRegularPath.GetAbsolutePath().c_str(), 14) });
		m_fontMapping.insert({ "arial", io.Fonts->AddFontFromFileTTF(arialPath.GetAbsolutePath().c_str(), 15) });
		m_fontMapping.insert({ "consolas", io.Fonts->AddFontFromFileTTF(consolasPath.GetAbsolutePath().c_str(), 14) });
		m_fontMapping.insert({ "liberation_bold", io.Fonts->AddFontFromFileTTF(liberationRegularPath.GetAbsolutePath().c_str(), 15) });
		m_fontMapping.insert({ "din_bold", io.Fonts->AddFontFromFileTTF(dinBoldPath.GetAbsolutePath().c_str(), 14) });

		io.Fonts->Build();
	}

	void EditorApp::StyleSetup()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 0.0f;
		style.WindowRounding = 0.0f;

		style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.46f, 0.46f, 0.46f, 0.54f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.67f, 0.67f, 0.67f, 0.40f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_Tab] = ImVec4(0.17f, 0.17f, 0.17f, 0.86f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
		style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}

	void EditorApp::AddFilePathToWindowTitle(const std::string& path)
	{
		std::ostringstream ss;
		ss << "RZEStudio - ";
		ss << path;
		GetWindow()->SetTitle(ss.str());
	}

	void EditorApp::RunAssetCpy()
	{
		static Filepath assetCpyPath("AssetCpy.bat");
		{
			FILE* pipe = nullptr;
			pipe = _popen(assetCpyPath.GetAbsolutePath().c_str(), "rt");
			char buffer[2048];
			while (fgets(buffer, 2048, pipe))
			{
				DebugServices::Get().Trace(LogChannel::Build, buffer);
			}
		}
	}

	void EditorApp::LoadScene(const Filepath& filepath)
	{
		// @TODO should go away with editor event system
		ResetSelectedObject();

		RZE().GetActiveScene().Unload();
		if (!filepath.IsValid())
		{
			RZE().GetActiveScene().NewScene();
		}
		else
		{
			RZE().GetActiveScene().Deserialize(filepath);
		}

		CreateAndInitializeEditorCamera();
	}

	void EditorApp::ParseArguments(char** arguments, int count)
	{
		RZE_Application::ParseArguments(arguments, count);
	}
}
