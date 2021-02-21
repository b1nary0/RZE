#include <EditorApp.h>

#include <ECS/Systems/FreeCameraSystem.h>

// Needed for call to RZE().GetRenderer().GetDriverDevice()
// Should remove ASAP.
#include <Diotima/Driver/DX11/DX11GFXDevice.h>
#include <Diotima/Graphics/RenderTarget.h>

#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/Platform/FilePath.h>

#include <DebugUtils/DebugServices.h>

#include <ImGui/imgui.h>
#include <imGUI/imgui_impl_dx11.h>
#include <imGUI/imgui_impl_win32.h>
#include <Optick/optick.h>

#include <stdio.h>

namespace
{
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
	}

	EditorApp::~EditorApp()
	{
	}

	static ImFont* ubuntu = nullptr;
	void EditorApp::Initialize()
	{
		RZE_Application::Initialize();

		// #TODO
		// Should probably bake this into RZE::Application
		FilePath::SetDirectoryContext(EDirectoryContext::Tools);

		GetWindow()->SetTitle("RZEStudio");
		GetWindow()->Maximize();

		const Vector2D& windowDims = GetWindow()->GetDimensions();
		mRenderTarget = std::make_unique<Diotima::RenderTargetTexture>(static_cast<U32>(windowDims.X()), static_cast<U32>(windowDims.Y()));
		mRenderTarget->Initialize(&RZE().GetLegacyRenderer().GetDriverDevice());
		RZE().GetLegacyRenderer().SetRenderTarget(mRenderTarget.get());

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::GetIO().KeyRepeatDelay = 0.5f;

		LoadFonts();
		StyleSetup();

		RZE().GetActiveScene().GetEntityHandler().AddSystem<FreeCameraSystem>();
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

		RZE().GetActiveScene().Load(FilePath("Assets/Scenes/TestGame.scene"));
	}

	void EditorApp::Update()
	{
		OPTICK_EVENT();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		Vector2D windowDims = GetWindow()->GetDimensions();

		ImGui::PushFont(mFontMapping.at("din_bold"));
		DisplayMenuBar();
		ImGui::SetNextWindowSize(ImVec2(windowDims.X(), windowDims.Y()));
		ImGui::SetNextWindowPos(ImVec2(0.f, kMenuBarHeight));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		bool show = true;
		ImGui::Begin("DockSpace Demo", &show, window_flags);
		ImGui::PopStyleVar(1);
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
			ImGui::DockSpace(dockspace_id, ImVec2(windowDims.X(), windowDims.Y()), dockspace_flags);

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
		//io.MouseWheel = static_cast<float>(handler.GetProxyMouseState().CurWheelVal);

		return mSceneViewPanel.IsHovered();
	}

	void EditorApp::SetFont(const char* fontName)
	{
		ImGui::PopFont();
		ImGui::PushFont(mFontMapping.at(fontName));
	}

	void EditorApp::Log(const std::string& msg)
	{
		mLogPanel.AddEntry(msg);
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

					RZE().GetActiveScene().Unload();
					RZE().GetActiveScene().NewScene();
				}
				if (ImGui::MenuItem("Load Scene..."))
				{
					FilePath newScenePath = RZE_Application::RZE().ShowOpenFilePrompt();
					if (newScenePath.IsValid())
					{
						RZE().GetActiveScene().Unload();
						// #TODO
						// Having to re-construct a FilePath like this seems weird.
						// Investigate. 
						// #NOTE
						// We should work mostly with relative paths, and resolve to absolute only when dealing with
						// lower-level clients of the API
						RZE().GetActiveScene().Load(FilePath(newScenePath.GetRelativePath()));
					}
				}
				if (ImGui::MenuItem("Save Scene"))
				{
					if (gEditorState.IsNewScene)
					{
						// #TODO
						// Using ShowOpenFilePrompt() here requires that you create the file
						// from within the open file prompt. This is a quick way to implement this feature,
						// but we need to fix this to use a new file prompt.
						FilePath newScenePath = RZE_Application::RZE().ShowOpenFilePrompt();
						if (newScenePath.IsValid())
						{
							RZE().GetActiveScene().Save(newScenePath);
							gEditorState.IsNewScene = false;
							RunAssetCpy();
						}
					}
					else
					{
						RZE().GetActiveScene().Save(FilePath());
						RunAssetCpy();
					}
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Build Game..."))
				{
					DebugServices::Get().Trace(LogChannel::Build, "Building Game...");
					Perseus::Job::Task buildTask = [this]()
					{
						char buffer[2048];
						FILE* pipe = nullptr;
						static FilePath buildGameBat("BuildGame.bat");
						pipe = _popen(buildGameBat.GetAbsolutePath().c_str(), "rt");
						while (fgets(buffer, 2048, pipe))
						{
							Log(buffer);
						}
					};
					Perseus::JobScheduler::Get().PushJob(buildTask);
				}
				if (ImGui::MenuItem("Launch Game..."))
				{
					Perseus::Job::Task gameTask = [this]()
					{
						static FilePath buildGameBat("BuildGame.bat");
						static FilePath assetCpyPath("AssetCpy.bat");
						static FilePath gamePath("_Build\\Debug\\x64\\RZE_Game.exe");

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
							FILE* pipe = nullptr;
							pipe = _popen(assetCpyPath.GetAbsolutePath().c_str(), "rt");
							char buffer[2048];
							while (fgets(buffer, 2048, pipe))
							{
								DebugServices::Get().Trace(LogChannel::Build, buffer);
							}
						}
						{
							FILE* pipe = nullptr;
							pipe = _popen(gamePath.GetAbsolutePath().c_str(), "rt");
						}
					};
					Perseus::JobScheduler::Get().PushJob(gameTask);

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
						mScenePanel.Enable();
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("Show ImGui Demo Window"))
				{
					mPanelStates.bDemoPanelEnabled = true;
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();
			static int frameCount = 0;
			static float frametime = 0.0f;
			if (frameCount > 15)
			{
				frametime = static_cast<float>(RZE().GetDeltaTime());
				frameCount = 0;
			}
			else
			{
				++frameCount;
			}

			ImGui::Text("%.1f ms", frametime * 1000.0f);

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
						mScenePanel.Enable();
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Create Entity"))
			{
				// #TODO(Do this better)
				static int sGenericEntityCount = 0;

				std::string newEntityStr = StringUtils::FormatString("Entity%i", sGenericEntityCount);
				RZE().GetActiveScene().CreateEntity(newEntityStr);

				++sGenericEntityCount;
			}

			ImGui::EndPopup();
		}
	}

	void EditorApp::ResolvePanelState()
	{
		if (mPanelStates.bDemoPanelEnabled)
		{
			ImGui::ShowDemoWindow(nullptr);
		}

		mScenePanel.Display();

		mSceneViewPanel.Display();

		mLogPanel.Display();
	}

	void EditorApp::LoadFonts()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();

		FilePath ubuntuRegularPath("Assets/Fonts/Ubuntu-Regular.ttf");
		FilePath ubuntuMediumPath("Assets/Fonts/Ubuntu-Medium.ttf");
		FilePath arialPath("Assets/Fonts/Arial.ttf");
		FilePath consolasPath("Assets/Fonts/Consolas.ttf");
		FilePath liberationRegularPath("Assets/Fonts/LiberationMono-Bold.ttf");
		FilePath dinBoldPath("Assets/Fonts/D-DIN-Bold.otf");

		mFontMapping.insert({"ubuntu_medium", io.Fonts->AddFontFromFileTTF(ubuntuMediumPath.GetAbsolutePath().c_str(), 16)});
		mFontMapping.insert({"ubuntu_regular", io.Fonts->AddFontFromFileTTF(ubuntuRegularPath.GetAbsolutePath().c_str(), 14)});
		mFontMapping.insert({"arial", io.Fonts->AddFontFromFileTTF(arialPath.GetAbsolutePath().c_str(), 15)});
		mFontMapping.insert({"consolas", io.Fonts->AddFontFromFileTTF(consolasPath.GetAbsolutePath().c_str(), 14)});
		mFontMapping.insert({ "liberation_bold", io.Fonts->AddFontFromFileTTF(liberationRegularPath.GetAbsolutePath().c_str(), 15) });
		mFontMapping.insert({ "din_bold", io.Fonts->AddFontFromFileTTF(dinBoldPath.GetAbsolutePath().c_str(), 14) });

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

	void EditorApp::RunAssetCpy()
	{
		static FilePath assetCpyPath("AssetCpy.bat");
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

	void EditorApp::ParseArguments(const char* arguments, int count)
	{
	}
}
