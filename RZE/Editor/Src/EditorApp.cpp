#include <EditorApp.h>

#include <ECS/Systems/FreeCameraSystem.h>

// Needed for call to RZE().GetRenderer().GetDriverDevice()
// Should remove ASAP.
#include <Diotima/Driver/DX11/DX11GFXDevice.h>
#include <Diotima/Graphics/RenderTarget.h>

#include <ImGui/imgui.h>
#include <imGUI/imgui_impl_dx11.h>
#include <imGUI/imgui_impl_win32.h>
#include <Optick/optick.h>

namespace Editor
{
	EditorApp::EditorApp()
		: bUIDisabled(false)
	{
	}

	EditorApp::~EditorApp()
	{
	}

	static ImFont* ubuntu = nullptr;
	void EditorApp::Initialize()
	{
		RZE_Application::Initialize();

		GetWindow()->SetTitle("RZEStudio");
		GetWindow()->Maximize();

		const Vector2D& windowDims = GetWindow()->GetDimensions();
		mRenderTarget = std::make_unique<Diotima::RenderTargetTexture>(static_cast<U32>(windowDims.X()), static_cast<U32>(windowDims.Y()));
		mRenderTarget->Initialize(&RZE().GetRenderer().GetDriverDevice());
		RZE().GetRenderer().SetRenderTarget(mRenderTarget.get());

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::GetIO().KeyRepeatDelay = 0.5f;

		LoadFonts();
		StyleSetup();

		RZE().GetActiveScene().GetEntityHandler().AddSystem<FreeCameraSystem>();
	}

	void EditorApp::Start()
	{
		RZE().GetActiveScene().Load(FilePath("Assets/Scenes/TestGame.scene"));
	}

	void EditorApp::Update()
	{
		OPTICK_EVENT();

		if (!UIDisabled())
		{
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			Vector2D windowDims = GetWindow()->GetDimensions();
			ImGui::SetNextWindowSize(ImVec2(windowDims.X(), windowDims.Y()));

			bool show = true;
			if (ImGui::Begin("DockSpace Demo", &show, window_flags))
			{
				ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
				ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
				ImGui::DockSpace(dockspace_id, ImVec2(windowDims.X(), windowDims.Y()), dockspace_flags);

				ImGui::PushFont(mFontMapping.at("liberation_bold"));

				DisplayMenuBar();
				//HandleGeneralContextMenu();

				ResolvePanelState();
				ImGui::PopFont();
			}
			ImGui::End();
		}
	}

	void EditorApp::ShutDown()
	{
	}

	void EditorApp::RegisterInputEvents(InputHandler& inputHandler)
	{
		Functor<void, const InputKey&> keyFunc([this](const InputKey & key)
		{
			if (key.GetKeyCode() == Win32KeyCode::F1)
			{
				DisableUI(!bUIDisabled);
			}
		});
		inputHandler.BindAction(Win32KeyCode::F1, EButtonState::ButtonState_Pressed, keyFunc);
	}

	void EditorApp::SetFont(const char* fontName)
	{
		ImGui::PopFont();
		ImGui::PushFont(mFontMapping.at(fontName));
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
					RZE().GetActiveScene().NewScene();
				}

				if (ImGui::MenuItem("Load Scene..."))
				{
					FilePath newScenePath = RZE_Application::RZE().ShowOpenFilePrompt();
					if (newScenePath.IsValid())
					{
						RZE().GetActiveScene().Load(newScenePath);
					}
				}

				if (ImGui::MenuItem("Save Scene"))
				{

				}

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
		ImGui::ShowDemoWindow(&mPanelStates.bDemoPanelEnabled);

		mScenePanel.Display();

		mSceneViewPanel.Display();
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

		mFontMapping.insert({"ubuntu_medium", io.Fonts->AddFontFromFileTTF(ubuntuMediumPath.GetAbsolutePath().c_str(), 16)});
		mFontMapping.insert({"ubuntu_regular", io.Fonts->AddFontFromFileTTF(ubuntuRegularPath.GetAbsolutePath().c_str(), 14)});
		mFontMapping.insert({"arial", io.Fonts->AddFontFromFileTTF(arialPath.GetAbsolutePath().c_str(), 15)});
		mFontMapping.insert({"consolas", io.Fonts->AddFontFromFileTTF(consolasPath.GetAbsolutePath().c_str(), 14)});
		mFontMapping.insert({ "liberation_bold", io.Fonts->AddFontFromFileTTF(liberationRegularPath.GetAbsolutePath().c_str(), 15) });

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

	void EditorApp::DisableUI(bool enable)
	{
		bUIDisabled = enable;
	}

	bool EditorApp::UIDisabled()
	{
		return bUIDisabled;
	}

}
