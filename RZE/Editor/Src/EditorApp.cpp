#include <EditorApp.h>

#include <Services/AssetFiles.h>

#include <UI/Framework/EditorUI.h>

#include <Asset/AssetPaths.h>

#include <Game/World/GameObject/EditorGameComponentInfoCache.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <Graphics/RenderEngine.h>
#include <Graphics/RenderStages/ImGuiRenderStage.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Input/ImGuiInput.h>

#include <Utils/Platform/CmdLine.h>
#include <Utils/Platform/Filepath.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h> // DockBuilderGetNode

#include <Optick/optick.h>

#include <sstream>

namespace
{
	constexpr char kSceneFileToLoadHack[] = { "RenderTest.scene" };

	// ImGui 1.92+ PushFont() takes a size; use the size the font was loaded at.
	void PushFontAtLoadedSize(ImFont* font)
	{
		ImGui::PushFont(font, font->LegacySize);
	}
}

namespace Editor
{
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
		// The main menu bar is the title bar
		GetWindow()->SetCustomTitleBar(true);
		GetWindow()->Maximize();

		const Vector2D& windowDims = GetWindow()->GetClientSize();
		CreateRenderTarget(windowDims);
		RZE().GetRenderEngine().GetMainView().Target = m_renderTarget.get();

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::GetIO().KeyRepeatDelay = 0.5f;

		LoadFonts();
		StyleSetup();

		m_mainMenuBar.Initialize();

		m_context.GetPanels().CreatePanels(m_context);

		// Nothing can restore what's left from a session that didn't close cleanly
		AssetFiles::EmptyTrash();
		m_context.GetProjectWatcher().Start();

#ifdef _DEBUG
		// Undo only sees changes made through commands, so catch anything that changes the scene's objects another way
		RZE().GetActiveScene().SetObjectsChangedCallback([this]()
			{
				AssertMsg(m_context.GetCommands().IsSceneEditOpen() || m_context.GetSceneDocument().IsLoading(),
					"Scene objects were added or removed outside a command; in the Editor, change them through one");
			});
#endif

		EditorEvents& events = m_context.GetEvents();
		m_titleSubscriptions.push_back(events.Subscribe<SceneLoadedEvent>([this](const SceneLoadedEvent& event)
			{
				RefreshWindowTitle(event.Document);
			}));
		m_titleSubscriptions.push_back(events.Subscribe<SceneSavedEvent>([this](const SceneSavedEvent& event)
			{
				RefreshWindowTitle(event.Document);
			}));
		m_titleSubscriptions.push_back(events.Subscribe<SceneModifiedStateChangedEvent>([this](const SceneModifiedStateChangedEvent& event)
			{
				RefreshWindowTitle(event.Document);
			}));
	}

	void EditorApp::Start()
	{
		// #TODO Reopen the scene that was open when the editor last closed, rather than a fixed default
		std::string_view startScene;
		const Filepath scenePath = AssetPaths::GetScene(CmdLine::Arguments::Get("-scene", startScene)
			? std::string(startScene)
			: std::string(kSceneFileToLoadHack));
		m_context.GetSceneDocument().Load(scenePath);
	}

	void EditorApp::PreUIUpdate()
	{
		OPTICK_EVENT();

		m_context.Update();
		m_context.GetPanels().UpdateAll(m_context);
	}

	void EditorApp::PreRender()
	{
		OPTICK_EVENT();

		m_context.GetPanels().PreRenderAll(m_context);
	}

	void EditorApp::UpdateUI()
	{
		OPTICK_EVENT();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		// Before anything draws, so the status bar and the panels see the same tasks
		m_context.GetTasks().Update(RZE().GetAsyncOperationManager().GetVisibleOperations());

		PushFontAtLoadedSize(m_fontMapping.at("din_bold"));
		m_mainMenuBar.Draw(m_context, *GetWindow());
		m_statusBar.Draw(m_context);
		// Before the main dockspace's panels, which may be docked in it
		m_bottomDockArea.Draw(m_context);

		// What the menu bar, status bar and bottom dock area leave of the window
		const ImGuiViewport* const viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		bool show = true;
		ImGui::Begin("RZE_Editor", &show, window_flags);
		ImGui::PopStyleVar(1);
		{
			// No node means imgui.ini had no saved layout for it (missing, or saved by an incompatible ImGui version)
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
			{
				m_context.GetPanels().BuildDefaultDockLayout(dockspace_id, ImGui::GetContentRegionAvail());
			}

			ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

			m_context.GetPanels().DrawAll(m_context);
		}
		ImGui::PopFont();
		ImGui::End();

		// Once every panel has drawn, so none of them sees the scene change partway through the frame
		m_context.GetCommands().Flush(m_context);

		PushFontAtLoadedSize(m_fontMapping.at("din_bold"));
		m_asyncOperationModal.Display(RZE().GetAsyncOperationManager());
		m_context.GetUnsavedChangesPrompt().Display(m_context);
		m_context.GetFileBrowser().Display();
		ImGui::PopFont();
	}

	bool EditorApp::OnCloseRequested()
	{
		if (!m_context.GetSceneDocument().IsModified())
		{
			return true;
		}

		m_context.GetUnsavedChangesPrompt().Request(m_context, "exiting", []() { RZE().PostExit(); });
		return false;
	}

	void EditorApp::ShutDown()
	{
		RZE().GetActiveScene().SetObjectsChangedCallback(nullptr);
		// While the renderer and resources are still up
		m_context.GetThumbnails().ShutDown();
		m_mainMenuBar.ShutDown();
		// Undo can't restore deleted files once the editor closes
		AssetFiles::EmptyTrash();
	}

	void EditorApp::RegisterInputEvents(InputHandler& inputHandler)
	{
		// NOTE: This will only get called when we're hovering the SceneView panel
		// SEE: EditorApp::ProcessInput return statement. 
		Functor<void, const InputKey&> keyFunc([this](const InputKey& key)
			{
				if (key.GetKeyCode() == Win32KeyCode::Escape)
				{
					EditorSelection& selection = m_context.GetSelection();
					if (!selection.IsEmpty())
					{
						selection.Clear();
					}
				}
			});
		inputHandler.BindAction(Win32KeyCode::Escape, EButtonState::ButtonState_Pressed, keyFunc);
	}

	bool EditorApp::ProcessInput(const InputHandler& handler)
	{
		ImGuiInput::SubmitInput(handler);

		return m_context.GetSceneViewport().IsHovered;
	}

	void EditorApp::OnWindowResize(const Vector2D& newSize)
	{
		CreateRenderTarget(newSize);
		RZE().GetRenderEngine().GetMainView().Target = m_renderTarget.get();
	}

	void EditorApp::CreateRenderTarget(const Vector2D& dimensions)
	{
		// #TODO pretty sure this causes a leak. doesn't look like the default destructors
		// are set up to do any cleanup. investigate.
		m_renderTarget.reset();

		m_renderTarget = std::make_unique<Rendering::RenderTargetTexture>(static_cast<U32>(dimensions.X()), static_cast<U32>(dimensions.Y()));
		m_renderTarget->Initialize();
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
	}

	void EditorApp::StyleSetup()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		// Everything ImGui draws with corners. Scrollbars and tabs keep ImGui's own, rounder ones.
		style.WindowRounding = EditorUI::k_cornerRounding;
		style.ChildRounding = EditorUI::k_cornerRounding;
		style.PopupRounding = EditorUI::k_cornerRounding;
		style.FrameRounding = EditorUI::k_cornerRounding;
		style.GrabRounding = EditorUI::k_cornerRounding;
		style.ImageRounding = EditorUI::k_cornerRounding;
		style.MenuItemRounding = EditorUI::k_cornerRounding;
		style.SelectableRounding = EditorUI::k_cornerRounding;

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
		style.Colors[ImGuiCol_TabSelected] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
		style.Colors[ImGuiCol_TabDimmed] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
		style.Colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
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

	void EditorApp::RefreshWindowTitle(const SceneDocument& document)
	{
		std::ostringstream ss;
		ss << "RZEStudio - ";
		ss << (document.HasFilePath() ? document.GetFilePath().GetRelativePath() : std::string("Untitled"));
		if (document.IsModified())
		{
			ss << "*";
		}
		GetWindow()->SetTitle(ss.str());
	}

	void EditorApp::ParseArguments(char** arguments, int count)
	{
		RZE_Application::ParseArguments(arguments, count);
	}
}
